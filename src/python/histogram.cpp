// Copyright 2015-2016 Hans Dembinski
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "serialization_suite.hpp"
#include "utility.hpp"
#include <boost/histogram/dynamic_histogram.hpp>
#include <boost/histogram/ostream_operators.hpp>
#include <boost/histogram/serialization.hpp>
#include <boost/python.hpp>
#include <boost/python/raw_function.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/variant/apply_visitor.hpp>
#include <boost/variant/static_visitor.hpp>
#include <boost/mpl/for_each.hpp>
#ifdef HAVE_NUMPY
#include <boost/python/numpy.hpp>
namespace np = boost::python::numpy;
#endif
#include <memory>

#ifndef BOOST_HISTOGRAM_AXIS_LIMIT
#define BOOST_HISTOGRAM_AXIS_LIMIT 32
#endif

namespace boost {

namespace histogram {
using histogram = dynamic_histogram<axis::builtins, adaptive_storage>;
} // namespace histogram

namespace python {

#ifdef HAVE_NUMPY
class access {
public:
  using mp_int = histogram::detail::mp_int;
  using weight_counter = histogram::detail::weight_counter;
  template <typename T>
  using array = histogram::detail::array<T>;

  struct dtype_visitor : public static_visitor<str> {
    list & shapes, & strides;
    dtype_visitor(list &sh, list &st) : shapes(sh), strides(st) {}
    template <typename T>
    str operator()(const array<T>& /*unused*/) const {
      strides.append(sizeof(T));
      return dtype_typestr<T>();
    }
    str operator()(const array<void>& /*unused*/) const {
      strides.append(sizeof(uint8_t));
      return dtype_typestr<uint8_t>();
    }
    str operator()(const array<mp_int>& /*unused*/) const {
      strides.append(sizeof(double));
      return dtype_typestr<double>();
    }
    str operator()(const array<weight_counter>& /*unused*/) const {
      strides.append(sizeof(double));
      strides.append(strides[-1] * 2);
      shapes.append(2);
      return dtype_typestr<double>();
    }
  };

  struct data_visitor : public static_visitor<object> {
    const list& shapes;
    const list& strides;
    data_visitor(const list& sh, const list& st) : shapes(sh), strides(st) {}
    template <typename Array>
    object operator()(const Array& b) const {
      return make_tuple(reinterpret_cast<uintptr_t>(b.begin()), true);
    }
    object operator()(const array<void>& b) const {
      // cannot pass non-existent memory to numpy; make new
      // zero-initialized uint8 array, and pass it
      return np::zeros(tuple(shapes), np::dtype::get_builtin<uint8_t>());
    }
    object operator()(const array<mp_int>& b) const {
      // cannot pass cpp_int to numpy; make new
      // double array, fill it and pass it
      auto a = np::empty(tuple(shapes), np::dtype::get_builtin<double>());
      for (auto i = 0l, n = len(shapes); i < n; ++i)
        const_cast<Py_intptr_t*>(a.get_strides())[i] = python::extract<int>(strides[i]);
      auto *buf = (double *)a.get_data();
      for (auto i = 0ul; i < b.size; ++i)
        buf[i] = static_cast<double>(b[i]);
      return a;
    }
  };

  static object array_interface(const histogram::histogram &self) {
    dict d;
    list shapes;
    list strides;
    auto &b = self.storage_.buffer_;
    d["typestr"] = apply_visitor(dtype_visitor(shapes, strides), b);
    for (auto i = 0u; i < self.dim(); ++i) {
      if (i) strides.append(strides[-1] * shapes[-1]);
      shapes.append(self.axis(i).shape());
    }
    if (self.dim() == 0)
      shapes.append(0);
    d["shape"] = tuple(shapes);
    d["strides"] = tuple(strides);
    d["data"] = apply_visitor(data_visitor(shapes, strides), b);
    return d;
  }
};
#endif

} // namespace python

namespace histogram {

struct axis_visitor : public static_visitor<python::object> {
  template <typename T> python::object operator()(const T &t) const {
    return python::object(t);
  }
};

struct axes_appender {
  python::object obj;
  std::vector<histogram::any_axis_type>& axes;
  bool& success;
  axes_appender(python::object o, std::vector<histogram::any_axis_type>& a,
                bool& s) : obj(o), axes(a), success(s) {}
  template <typename A> void operator()(const A&) const {
    if (success) return;
    python::extract<const A&> get_axis(obj);
    if (get_axis.check()) {
      axes.emplace_back(get_axis());
      success = true;
    }
  }
};

python::object histogram_axis(const histogram &self, int i) {
  if (i < 0)
    i += self.dim();
  if (i < 0 || i >= int(self.dim())) {
    PyErr_SetString(PyExc_IndexError, "axis index out of range");
    python::throw_error_already_set();
  }
  return apply_visitor(axis_visitor(), self.axis(i));
}

python::object histogram_init(python::tuple args, python::dict kwargs) {

  python::object self = args[0];
  python::object pyinit = self.attr("__init__");

  if (kwargs) {
    PyErr_SetString(PyExc_RuntimeError, "no keyword arguments allowed");
    python::throw_error_already_set();
  }

  const unsigned dim = len(args) - 1;

  // normal constructor
  std::vector<histogram::any_axis_type> axes;
  for (unsigned i = 0; i < dim; ++i) {
    python::object pa = args[i + 1];
    bool success = false;
    mpl::for_each<histogram::any_axis_type::types>(
      axes_appender(pa, axes, success)
    );
    if (!success) {
      std::string msg = "require an axis object, got ";
      msg += python::extract<std::string>(pa.attr("__class__"))();
      PyErr_SetString(PyExc_TypeError, msg.c_str());
      python::throw_error_already_set();
    }
  }
  histogram h(axes.begin(), axes.end());
  return pyinit(h);
}

template <typename T>
struct fetcher {
  long n = -1;
  union {
    T value = 0;
    const T* carray;
  };
  python::object keep_alive;

  void assign(python::object o) {
    // skipping check for currently held type, since it is always value
    python::extract<T> get_value(o);
    if (get_value.check()) {
      value = get_value();
      n = 0;
      return;
    }
#ifdef HAVE_NUMPY
    np::ndarray a = np::from_object(o, np::dtype::get_builtin<T>(), 1);
    carray = reinterpret_cast<const T*>(a.get_data());
    n = a.shape(0);
    keep_alive = a; // this may be a temporary object
    return;
#endif
    throw std::invalid_argument("argument must be a number");
  }
  T get(long i) const noexcept {
    if (n > 0)
      return carray[i];
    return value;
  }
};

python::object histogram_fill(python::tuple args, python::dict kwargs) {
  const auto nargs = python::len(args);
  histogram &self = python::extract<histogram &>(args[0]);

  const unsigned dim = nargs - 1;
  if (dim != self.dim()) {
    PyErr_SetString(PyExc_ValueError, "number of arguments and dimension do not match");
    python::throw_error_already_set();
  }

  if (dim > BOOST_HISTOGRAM_AXIS_LIMIT) {
    std::ostringstream os;
    os << "too many axes, maximum is " << BOOST_HISTOGRAM_AXIS_LIMIT;
    PyErr_SetString(PyExc_RuntimeError, os.str().c_str());
    python::throw_error_already_set();
  }

  fetcher<double> fetch[BOOST_HISTOGRAM_AXIS_LIMIT];
  long n = 0;
  for (auto d = 0u; d < dim; ++d) {
    fetch[d].assign(args[1 + d]);
    if (fetch[d].n > 0) {
      if (n > 0 && fetch[d].n != n) {
        PyErr_SetString(PyExc_ValueError, "lengths of sequences do not match");
        python::throw_error_already_set();
      }
      n = fetch[d].n;
    }
  }

  fetcher<double> fetch_weight;
  fetcher<unsigned> fetch_count;
  const auto nkwargs = python::len(kwargs);
  if (nkwargs > 0) {
    const bool use_weight = kwargs.has_key("weight");
    const bool use_count = kwargs.has_key("count");
    if (nkwargs > 1 || (use_weight == use_count)) { // may not be both true or false
      PyErr_SetString(PyExc_RuntimeError, "only keyword weight or count allowed");
      python::throw_error_already_set();
    }

    if (use_weight) {
      fetch_weight.assign(kwargs.get("weight"));
      if (fetch_weight.n > 0) {
        if (n > 0 && fetch_weight.n != n) {
          PyErr_SetString(PyExc_ValueError, "length of weight sequence does not match");
          python::throw_error_already_set();
        }
        n = fetch_weight.n;
      }
    }

    if (use_count) {
      fetch_count.assign(kwargs.get("count"));
      if (fetch_count.n > 0) {
        if (n > 0 && fetch_count.n != n) {
          PyErr_SetString(PyExc_ValueError, "length of count sequence does not match");
          python::throw_error_already_set();
        }
        n = fetch_count.n;
      }
    }
  }

  double v[BOOST_HISTOGRAM_AXIS_LIMIT];
  if (!n) ++n;
  for (auto i = 0l; i < n; ++i) {
    for (auto d = 0u; d < dim; ++d)
      v[d] = fetch[d].get(i);
    if (fetch_weight.n >= 0)
      self.fill(v, v + dim, weight(fetch_weight.get(i)));
    else if (fetch_count.n >= 0)
      self.fill(v, v + dim, count(fetch_count.get(i)));
    else
      self.fill(v, v + dim);
  }

  return python::object();
}

python::object histogram_value(python::tuple args, python::dict kwargs) {
  const histogram & self = python::extract<const histogram &>(args[0]);

  const unsigned dim = len(args) - 1;
  if (self.dim() != dim) {
    PyErr_SetString(PyExc_RuntimeError, "wrong number of arguments");
    python::throw_error_already_set();
  }

  if (dim > BOOST_HISTOGRAM_AXIS_LIMIT) {
    std::ostringstream os;
    os << "too many axes, maximum is " << BOOST_HISTOGRAM_AXIS_LIMIT;
    PyErr_SetString(PyExc_RuntimeError, os.str().c_str());
    python::throw_error_already_set();
  }

  if (kwargs) {
    PyErr_SetString(PyExc_RuntimeError, "no keyword arguments allowed");
    python::throw_error_already_set();
  }

  int idx[BOOST_HISTOGRAM_AXIS_LIMIT];
  for (unsigned i = 0; i < self.dim(); ++i)
    idx[i] = python::extract<int>(args[1 + i]);

  return python::object(self.value(idx, idx + self.dim()));
}

python::object histogram_variance(python::tuple args, python::dict kwargs) {
  const histogram &self =
      python::extract<const histogram &>(args[0]);

  const unsigned dim = len(args) - 1;
  if (self.dim() != dim) {
    PyErr_SetString(PyExc_RuntimeError, "wrong number of arguments");
    python::throw_error_already_set();
  }

  if (dim > BOOST_HISTOGRAM_AXIS_LIMIT) {
    std::ostringstream os;
    os << "too many axes, maximum is " << BOOST_HISTOGRAM_AXIS_LIMIT;
    PyErr_SetString(PyExc_RuntimeError, os.str().c_str());
    python::throw_error_already_set();
  }

  if (kwargs) {
    PyErr_SetString(PyExc_RuntimeError, "no keyword arguments allowed");
    python::throw_error_already_set();
  }

  int idx[BOOST_HISTOGRAM_AXIS_LIMIT];
  for (auto i = 0u; i < self.dim(); ++i)
    idx[i] = python::extract<int>(args[1 + i]);

  return python::object(self.variance(idx, idx + self.dim()));
}

python::object histogram_reduce_to(python::tuple args, python::dict kwargs) {
  const histogram &self =
      python::extract<const histogram &>(args[0]);

  const unsigned nargs = len(args) - 1;

  if (nargs > BOOST_HISTOGRAM_AXIS_LIMIT) {
    std::ostringstream os;
    os << "too many arguments, maximum is " << BOOST_HISTOGRAM_AXIS_LIMIT;
    PyErr_SetString(PyExc_RuntimeError, os.str().c_str());
    python::throw_error_already_set();
  }

  if (kwargs) {
    PyErr_SetString(PyExc_RuntimeError, "no keyword arguments allowed");
    python::throw_error_already_set();
  }

  int idx[BOOST_HISTOGRAM_AXIS_LIMIT];
  for (auto i = 0u; i < nargs; ++i)
    idx[i] = python::extract<int>(args[1 + i]);

  return python::object(self.reduce_to(idx, idx + nargs));
}

std::string histogram_repr(const histogram &h) {
  std::ostringstream os;
  os << h;
  return os.str();
}

void register_histogram() {
  python::docstring_options dopt(true, true, false);

  python::class_<histogram, boost::shared_ptr<histogram>>(
      "histogram", "N-dimensional histogram for real-valued data.", python::no_init)
      .def("__init__", python::raw_function(histogram_init),
           ":param axis args: axis objects"
           "\nPass one or more axis objects to configure the histogram.")
      // shadowed C++ ctors
      .def(python::init<const histogram &>())
#ifdef HAVE_NUMPY
      .add_property("__array_interface__", &python::access::array_interface)
#endif
      .add_property("dim", &histogram::dim)
      .def("axis", histogram_axis, python::arg("i") = 0,
           ":param int i: axis index"
           "\n:return: corresponding axis object")
      .def("fill", python::raw_function(histogram_fill),
           ":param double args: values (number must match dimension)"
           "\n:keyword double weight: optional weight"
           "\n:keyword uint32_t count: optional count"
           "\n"
           "\nIf Numpy support is enabled, 1d-arrays can be passed instead of"
           "\nvalues, which must be equal in lenght. Arrays and values can"
           "\nbe mixed arbitrarily in the same call.")
      .add_property("bincount", &histogram::bincount,
           ":return: total number of bins, including under- and overflow")
      .add_property("sum", &histogram::sum,
           ":return: sum of all entries, including under- and overflow bins")
      .def("value", python::raw_function(histogram_value),
           ":param int args: indices of the bin (number must match dimension)"
           "\n:return: count for the bin")
      .def("variance", python::raw_function(histogram_variance),
           ":param int args: indices of the bin (number must match dimension)"
           "\n:return: variance estimate for the bin")
      .def("reduce_to", python::raw_function(histogram_reduce_to),
           ":param int args: indices of the axes in the reduced histogram"
           "\n:return: reduced histogram with subset of axes")
      .def("__repr__", histogram_repr,
           ":return: string representation of the histogram")
      .def(python::self == python::self)
      .def(python::self += python::self)
      .def(python::self *= double())
      .def(python::self * double())
      .def(double() * python::self)
      .def(python::self + python::self)
      .def_pickle(serialization_suite<histogram>());
}

} // NS histogram
} // NS boost
