// Copyright 2015-2016 Hans Dembinski
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/core/lightweight_test.hpp>
#include <boost/histogram/histogram.hpp>
#include <boost/histogram/ostream_operators.hpp>
#include <boost/histogram/literals.hpp>
#include <boost/histogram/serialization.hpp>
#include <boost/histogram/storage/adaptive_storage.hpp>
#include <boost/histogram/storage/array_storage.hpp>
#include <boost/mpl/int.hpp>
#include <limits>
#include <sstream>
#include <vector>
#include <cstdlib>

using namespace boost::histogram;
using namespace boost::histogram::literals; // to get _c suffix
namespace mpl = boost::mpl;

using Static = mpl::int_<0>;
using Dynamic = mpl::int_<1>;

template <typename S, typename... Axes>
auto make_histogram(Static, Axes &&... axes)
    -> decltype(make_static_histogram_with<S>(std::forward<Axes>(axes)...)) {
  return make_static_histogram_with<S>(std::forward<Axes>(axes)...);
}

template <typename S, typename... Axes>
auto make_histogram(Dynamic, Axes &&... axes)
    -> decltype(make_dynamic_histogram_with<S>(std::forward<Axes>(axes)...)) {
  return make_dynamic_histogram_with<S>(std::forward<Axes>(axes)...);
}

template <typename T, typename U>
bool axis_equal(Static, const T &t, const U &u) {
  return t == u;
}

template <typename T, typename U>
bool axis_equal(Dynamic, const T &t, const U &u) {
  return t == T(u); // need to convert rhs to boost::variant
}

template <typename Type> void run_tests() {

  // init_0
  {
    auto h =
        static_histogram<mpl::vector<axis::integer<>>, adaptive_storage>();
    BOOST_TEST_EQ(h.dim(), 1);
    BOOST_TEST_EQ(h.bincount(), 0);
    auto h2 = static_histogram<mpl::vector<axis::integer<>>,
                        array_storage<unsigned>>();
    BOOST_TEST_EQ(h2, h);
    auto h3 =
        static_histogram<mpl::vector<axis::regular<>>, adaptive_storage>();
    BOOST_TEST_NE(h3, h);
  }

  // init_1
  {
    auto h =
        make_histogram<adaptive_storage>(Type(), axis::regular<>{3, -1, 1});
    BOOST_TEST_EQ(h.dim(), 1);
    BOOST_TEST_EQ(h.bincount(), 5);
    BOOST_TEST_EQ(h.axis(0_c).shape(), 5);
    BOOST_TEST_EQ(h.axis().shape(), 5);
    auto h2 = make_histogram<array_storage<unsigned>>(
        Type(), axis::regular<>{3, -1, 1});
    BOOST_TEST_EQ(h2, h);
  }

  // init_2
  {
    auto h = make_histogram<adaptive_storage>(Type(), axis::regular<>{3, -1, 1},
                                              axis::integer<>{-1, 2});
    BOOST_TEST_EQ(h.dim(), 2);
    BOOST_TEST_EQ(h.bincount(), 25);
    BOOST_TEST_EQ(h.axis(0_c).shape(), 5);
    BOOST_TEST_EQ(h.axis(1_c).shape(), 5);
    auto h2 = make_histogram<array_storage<unsigned>>(
        Type(), axis::regular<>{3, -1, 1}, axis::integer<>{-1, 2});
    BOOST_TEST_EQ(h2, h);
  }

  // init_3
  {
    auto h = make_histogram<adaptive_storage>(Type(), axis::regular<>{3, -1, 1},
                                              axis::integer<>{-1, 2},
                                              axis::circular<>{3});
    BOOST_TEST_EQ(h.dim(), 3);
    BOOST_TEST_EQ(h.bincount(), 75);
    auto h2 = make_histogram<array_storage<unsigned>>(
        Type(), axis::regular<>{3, -1, 1}, axis::integer<>{-1, 2},
        axis::circular<>{3});
    BOOST_TEST_EQ(h2, h);
  }

  // init_4
  {
    auto h = make_histogram<adaptive_storage>(
        Type(), axis::regular<>{3, -1, 1}, axis::integer<>{-1, 2},
        axis::circular<>{3}, axis::variable<>{-1, 0, 1});
    BOOST_TEST_EQ(h.dim(), 4);
    BOOST_TEST_EQ(h.bincount(), 300);
    auto h2 = make_histogram<array_storage<unsigned>>(
        Type(), axis::regular<>{3, -1, 1}, axis::integer<>{-1, 2},
        axis::circular<>{3}, axis::variable<>{-1, 0, 1});
    BOOST_TEST_EQ(h2, h);
  }

  // init_5
  {
    enum { A, B, C };
    auto h = make_histogram<adaptive_storage>(
        Type(), axis::regular<>{3, -1, 1}, axis::integer<>{-1, 2},
        axis::circular<>{3}, axis::variable<>{-1, 0, 1},
        axis::category<>{{A, B, C}});
    BOOST_TEST_EQ(h.dim(), 5);
    BOOST_TEST_EQ(h.bincount(), 900);
    auto h2 = make_histogram<array_storage<unsigned>>(
        Type(), axis::regular<>{3, -1, 1}, axis::integer<>{-1, 2},
        axis::circular<>{3}, axis::variable<>{-1, 0, 1},
        axis::category<>{{A, B, C}});
    BOOST_TEST_EQ(h2, h);
  }

  // copy_ctor
  {
    auto h = make_histogram<adaptive_storage>(Type(), axis::integer<>{0, 2},
                                              axis::integer<>{0, 3});
    h.fill(0, 0);
    auto h2 = decltype(h)(h);
    BOOST_TEST(h2 == h);
    auto h3 = static_histogram<mpl::vector<axis::integer<>, axis::integer<>>,
                        array_storage<unsigned>>(h);
    BOOST_TEST_EQ(h3, h);
  }

  // copy_assign
  {
    auto h = make_histogram<adaptive_storage>(Type(), axis::integer<>(0, 1),
                                              axis::integer<>(0, 2));
    h.fill(0, 0);
    auto h2 = decltype(h)();
    BOOST_TEST_NE(h, h2);
    h2 = h;
    BOOST_TEST_EQ(h, h2);
    // test self-assign
    h2 = h2;
    BOOST_TEST_EQ(h, h2);
    auto h3 = static_histogram<mpl::vector<axis::integer<>, axis::integer<>>,
                        array_storage<unsigned>>();
    h3 = h;
    BOOST_TEST_EQ(h, h3);
  }

  // move
  {
    auto h = make_histogram<adaptive_storage>(Type(), axis::integer<>(0, 1),
                                              axis::integer<>(0, 2));
    h.fill(0, 0);
    const auto href = h;
    decltype(h) h2(std::move(h));
    BOOST_TEST_EQ(h.dim(),
                  Type() == 0 ? 2 : 0); // static axes cannot shrink to zero
    BOOST_TEST_EQ(h.sum(), 0);
    BOOST_TEST_EQ(h.bincount(), 0);
    BOOST_TEST_EQ(h2, href);
    decltype(h) h3;
    h3 = std::move(h2);
    BOOST_TEST_EQ(h2.dim(),
                  Type() == 0 ? 2 : 0); // static axes cannot shrink to zero
    BOOST_TEST_EQ(h2.sum(), 0);
    BOOST_TEST_EQ(h2.bincount(), 0);
    BOOST_TEST_EQ(h3, href);
  }

  // axis methods
  {
    enum { A=3, B=5 };
    auto a = make_histogram<adaptive_storage>(Type(), axis::regular<>(1, 1, 2, "foo"));
    BOOST_TEST_EQ(a.axis().size(), 1);
    BOOST_TEST_EQ(a.axis().shape(), 3);
    BOOST_TEST_EQ(a.axis().index(1.0), 0);
    BOOST_TEST_EQ(a.axis()[0].lower(), 1.0);
    BOOST_TEST_EQ(a.axis()[0].upper(), 2.0);
    BOOST_TEST_EQ(a.axis().label(), "foo");
    a.axis().label("bar");
    BOOST_TEST_EQ(a.axis().label(), "bar");

    auto b = make_histogram<adaptive_storage>(Type(), axis::integer<>(1, 2));
    BOOST_TEST_EQ(b.axis().size(), 1);
    BOOST_TEST_EQ(b.axis().shape(), 3);
    BOOST_TEST_EQ(b.axis().index(1), 0);
    BOOST_TEST_EQ(b.axis()[0].lower(), 1);
    BOOST_TEST_EQ(b.axis()[0].upper(), 2);
    b.axis().label("foo");
    BOOST_TEST_EQ(b.axis().label(), "foo");

    auto c = make_histogram<adaptive_storage>(Type(), axis::category<>({A, B}));
    BOOST_TEST_EQ(c.axis().size(), 2);
    BOOST_TEST_EQ(c.axis().shape(), 2);
    BOOST_TEST_EQ(c.axis().index(A), 0);
    BOOST_TEST_EQ(c.axis().index(B), 1);
    c.axis().label("foo");
    BOOST_TEST_EQ(c.axis().label(), "foo");
    // need to cast here for this to work with Type == Dynamic
    auto ca = axis::cast<axis::category<>>(c.axis());
    BOOST_TEST_EQ(ca[0], A);
  }

  // equal_compare
  {
    auto a = make_histogram<adaptive_storage>(Type(), axis::integer<>(0, 2));
    auto b = make_histogram<adaptive_storage>(Type(), axis::integer<>(0, 2),
                                              axis::integer<>(0, 3));
    BOOST_TEST(a != b);
    BOOST_TEST(b != a);
    auto c = make_histogram<adaptive_storage>(Type(), axis::integer<>(0, 2));
    BOOST_TEST(b != c);
    BOOST_TEST(c != b);
    BOOST_TEST(a == c);
    BOOST_TEST(c == a);
    auto d = make_histogram<adaptive_storage>(Type(), axis::regular<>(2, 0, 1));
    BOOST_TEST(c != d);
    BOOST_TEST(d != c);
    c.fill(0);
    BOOST_TEST(a != c);
    BOOST_TEST(c != a);
    a.fill(0);
    BOOST_TEST(a == c);
    BOOST_TEST(c == a);
    a.fill(0);
    BOOST_TEST(a != c);
    BOOST_TEST(c != a);
  }

  // d1
  {
    auto h = make_histogram<adaptive_storage>(Type(), axis::integer<>{0, 2});
    h.fill(0);
    h.fill(0);
    h.fill(-1);
    h.fill(10, count(10));

    BOOST_TEST_EQ(h.dim(), 1);
    BOOST_TEST_EQ(h.axis(0_c).size(), 2);
    BOOST_TEST_EQ(h.axis(0_c).shape(), 4);
    BOOST_TEST_EQ(h.sum(), 13);

    BOOST_TEST_THROWS(h.value(-2), std::out_of_range);
    BOOST_TEST_EQ(h.value(-1), 1);
    BOOST_TEST_EQ(h.value(0), 2);
    BOOST_TEST_EQ(h.value(1), 0);
    BOOST_TEST_EQ(h.value(2), 10);
    BOOST_TEST_THROWS(h.value(3), std::out_of_range);

    BOOST_TEST_THROWS(h.variance(-2), std::out_of_range);
    BOOST_TEST_EQ(h.variance(-1), 1);
    BOOST_TEST_EQ(h.variance(0), 2);
    BOOST_TEST_EQ(h.variance(1), 0);
    BOOST_TEST_EQ(h.variance(2), 10);
    BOOST_TEST_THROWS(h.variance(3), std::out_of_range);
  }

  // d1_2
  {
    auto h = make_histogram<adaptive_storage>(
        Type(), axis::integer<>(0, 2, "", axis::uoflow::off));
    h.fill(0);
    h.fill(-0);
    h.fill(-1);
    h.fill(10, count(10));

    BOOST_TEST_EQ(h.dim(), 1);
    BOOST_TEST_EQ(h.axis(0_c).size(), 2);
    BOOST_TEST_EQ(h.axis(0_c).shape(), 2);
    BOOST_TEST_EQ(h.sum(), 2);

    BOOST_TEST_THROWS(h.value(-1), std::out_of_range);
    BOOST_TEST_EQ(h.value(0), 2);
    BOOST_TEST_EQ(h.value(1), 0);
    BOOST_TEST_THROWS(h.value(2), std::out_of_range);

    BOOST_TEST_THROWS(h.variance(-1), std::out_of_range);
    BOOST_TEST_EQ(h.variance(0), 2);
    BOOST_TEST_EQ(h.variance(1), 0);
    BOOST_TEST_THROWS(h.variance(2), std::out_of_range);
  }

  // d1_3
  {
    auto h = make_histogram<adaptive_storage>(
        Type(), axis::category<std::string>({"A", "B"}));
    h.fill("A");
    h.fill("B");
    h.fill("D");
    h.fill("E", count(10));

    BOOST_TEST_EQ(h.dim(), 1);
    BOOST_TEST_EQ(h.axis(0_c).size(), 2);
    BOOST_TEST_EQ(h.axis(0_c).shape(), 2);
    BOOST_TEST_EQ(h.sum(), 2);

    BOOST_TEST_THROWS(h.value(-1), std::out_of_range);
    BOOST_TEST_EQ(h.value(0), 1);
    BOOST_TEST_EQ(h.value(1), 1);
    BOOST_TEST_THROWS(h.value(2), std::out_of_range);

    BOOST_TEST_THROWS(h.variance(-1), std::out_of_range);
    BOOST_TEST_EQ(h.variance(0), 1);
    BOOST_TEST_EQ(h.variance(1), 1);
    BOOST_TEST_THROWS(h.variance(2), std::out_of_range);
  }

  // d1w
  {
    auto h =
        make_histogram<adaptive_storage>(Type(), axis::regular<>(2, -1, 1));
    h.fill(0);
    h.fill(weight(2.0), -1.0);
    h.fill(-1.0);
    h.fill(-2.0);
    h.fill(weight(5.0), 10.0);

    BOOST_TEST_EQ(h.sum(), 10);

    BOOST_TEST_EQ(h.value(-1), 1.0);
    BOOST_TEST_EQ(h.value(0), 3.0);
    BOOST_TEST_EQ(h.value(1), 1.0);
    BOOST_TEST_EQ(h.value(2), 5.0);

    BOOST_TEST_EQ(h.variance(-1), 1.0);
    BOOST_TEST_EQ(h.variance(0), 5.0);
    BOOST_TEST_EQ(h.variance(1), 1.0);
    BOOST_TEST_EQ(h.variance(2), 25.0);
  }

  // d1w2
  {
    auto h =
        make_histogram<array_storage<float>>(Type(), axis::regular<>(2, -1, 1));
    h.fill(0);
    h.fill(count(2.0), -1.0);
    h.fill(-1.0);
    h.fill(-2.0);
    h.fill(count(5.0), 10.0);

    BOOST_TEST_EQ(h.sum(), 10);

    BOOST_TEST_EQ(h.value(-1), 1.0);
    BOOST_TEST_EQ(h.value(0), 3.0);
    BOOST_TEST_EQ(h.value(1), 1.0);
    BOOST_TEST_EQ(h.value(2), 5.0);
  }

  // d2
  {
    auto h = make_histogram<adaptive_storage>(
        Type(), axis::regular<>(2, -1, 1),
        axis::integer<>(-1, 2, "", axis::uoflow::off));
    h.fill(-1, -1);
    h.fill(-1, 0);
    h.fill(-1, -10);
    h.fill(-10, 0);

    BOOST_TEST_EQ(h.dim(), 2);
    BOOST_TEST_EQ(h.axis(0_c).size(), 2);
    BOOST_TEST_EQ(h.axis(0_c).shape(), 4);
    BOOST_TEST_EQ(h.axis(1_c).size(), 3);
    BOOST_TEST_EQ(h.axis(1_c).shape(), 3);
    BOOST_TEST_EQ(h.sum(), 3);

    BOOST_TEST_EQ(h.value(-1, 0), 0.0);
    BOOST_TEST_EQ(h.value(-1, 1), 1.0);
    BOOST_TEST_EQ(h.value(-1, 2), 0.0);

    BOOST_TEST_EQ(h.value(0, 0), 1.0);
    BOOST_TEST_EQ(h.value(0, 1), 1.0);
    BOOST_TEST_EQ(h.value(0, 2), 0.0);

    BOOST_TEST_EQ(h.value(1, 0), 0.0);
    BOOST_TEST_EQ(h.value(1, 1), 0.0);
    BOOST_TEST_EQ(h.value(1, 2), 0.0);

    BOOST_TEST_EQ(h.value(2, 0), 0.0);
    BOOST_TEST_EQ(h.value(2, 1), 0.0);
    BOOST_TEST_EQ(h.value(2, 2), 0.0);

    BOOST_TEST_EQ(h.variance(-1, 0), 0.0);
    BOOST_TEST_EQ(h.variance(-1, 1), 1.0);
    BOOST_TEST_EQ(h.variance(-1, 2), 0.0);

    BOOST_TEST_EQ(h.variance(0, 0), 1.0);
    BOOST_TEST_EQ(h.variance(0, 1), 1.0);
    BOOST_TEST_EQ(h.variance(0, 2), 0.0);

    BOOST_TEST_EQ(h.variance(1, 0), 0.0);
    BOOST_TEST_EQ(h.variance(1, 1), 0.0);
    BOOST_TEST_EQ(h.variance(1, 2), 0.0);

    BOOST_TEST_EQ(h.variance(2, 0), 0.0);
    BOOST_TEST_EQ(h.variance(2, 1), 0.0);
    BOOST_TEST_EQ(h.variance(2, 2), 0.0);
  }

  // d2w
  {
    auto h = make_histogram<adaptive_storage>(
        Type(), axis::regular<>(2, -1, 1),
        axis::integer<>(-1, 2, "", axis::uoflow::off));
    h.fill(-1, 0);              // -> 0, 1
    h.fill(weight(10), -1, -1); // -> 0, 0
    h.fill(weight(5), -1, -10); // is ignored
    h.fill(weight(7), -10, 0);  // -> -1, 1

    BOOST_TEST_EQ(h.sum(), 18);

    BOOST_TEST_EQ(h.value(-1, 0), 0.0);
    BOOST_TEST_EQ(h.value(-1, 1), 7.0);
    BOOST_TEST_EQ(h.value(-1, 2), 0.0);

    BOOST_TEST_EQ(h.value(0, 0), 10.0);
    BOOST_TEST_EQ(h.value(0, 1), 1.0);
    BOOST_TEST_EQ(h.value(0, 2), 0.0);

    BOOST_TEST_EQ(h.value(1, 0), 0.0);
    BOOST_TEST_EQ(h.value(1, 1), 0.0);
    BOOST_TEST_EQ(h.value(1, 2), 0.0);

    BOOST_TEST_EQ(h.value(2, 0), 0.0);
    BOOST_TEST_EQ(h.value(2, 1), 0.0);
    BOOST_TEST_EQ(h.value(2, 2), 0.0);

    BOOST_TEST_EQ(h.variance(-1, 0), 0.0);
    BOOST_TEST_EQ(h.variance(-1, 1), 49.0);
    BOOST_TEST_EQ(h.variance(-1, 2), 0.0);

    BOOST_TEST_EQ(h.variance(0, 0), 100.0);
    BOOST_TEST_EQ(h.variance(0, 1), 1.0);
    BOOST_TEST_EQ(h.variance(0, 2), 0.0);

    BOOST_TEST_EQ(h.variance(1, 0), 0.0);
    BOOST_TEST_EQ(h.variance(1, 1), 0.0);
    BOOST_TEST_EQ(h.variance(1, 2), 0.0);

    BOOST_TEST_EQ(h.variance(2, 0), 0.0);
    BOOST_TEST_EQ(h.variance(2, 1), 0.0);
    BOOST_TEST_EQ(h.variance(2, 2), 0.0);
  }

  // d3w
  {
    auto h = make_histogram<adaptive_storage>(Type(), axis::integer<>(0, 3),
                                              axis::integer<>(0, 4),
                                              axis::integer<>(0, 5));
    for (auto i = 0; i < h.axis(0_c).size(); ++i) {
      for (auto j = 0; j < h.axis(1_c).size(); ++j) {
        for (auto k = 0; k < h.axis(2_c).size(); ++k) {
          h.fill(weight(i + j + k), i, j, k);
        }
      }
    }

    for (auto i = 0; i < h.axis(0_c).size(); ++i) {
      for (auto j = 0; j < h.axis(1_c).size(); ++j) {
        for (auto k = 0; k < h.axis(2_c).size(); ++k) {
          BOOST_TEST_EQ(h.value(i, j, k), i + j + k);
        }
      }
    }
  }

  // add_1
  {
    auto a = make_histogram<adaptive_storage>(Type(), axis::integer<>(-1, 2));
    auto b =
        make_histogram<array_storage<unsigned>>(Type(), axis::integer<>(-1, 2));
    a.fill(-1);
    b.fill(1);
    auto c = a;
    c += b;
    BOOST_TEST_EQ(c.value(-1), 0);
    BOOST_TEST_EQ(c.value(0), 1);
    BOOST_TEST_EQ(c.value(1), 0);
    BOOST_TEST_EQ(c.value(2), 1);
    BOOST_TEST_EQ(c.value(3), 0);
    auto d = a;
    d += b;
    BOOST_TEST_EQ(d.value(-1), 0);
    BOOST_TEST_EQ(d.value(0), 1);
    BOOST_TEST_EQ(d.value(1), 0);
    BOOST_TEST_EQ(d.value(2), 1);
    BOOST_TEST_EQ(d.value(3), 0);
  }

  // add_2
  {
    auto a = make_histogram<adaptive_storage>(Type(), axis::integer<>(0, 2));
    auto b = make_histogram<adaptive_storage>(Type(), axis::integer<>(0, 2));

    a.fill(0);
    BOOST_TEST_EQ(a.variance(0), 1);
    b.fill(1, weight(3));
    BOOST_TEST_EQ(b.variance(1), 9);
    auto c = a;
    c += b;
    BOOST_TEST_EQ(c.value(-1), 0);
    BOOST_TEST_EQ(c.value(0), 1);
    BOOST_TEST_EQ(c.variance(0), 1);
    BOOST_TEST_EQ(c.value(1), 3);
    BOOST_TEST_EQ(c.variance(1), 9);
    BOOST_TEST_EQ(c.value(2), 0);
    auto d = a;
    d += b;
    BOOST_TEST_EQ(d.value(-1), 0);
    BOOST_TEST_EQ(d.value(0), 1);
    BOOST_TEST_EQ(d.variance(0), 1);
    BOOST_TEST_EQ(d.value(1), 3);
    BOOST_TEST_EQ(d.variance(1), 9);
    BOOST_TEST_EQ(d.value(2), 0);
  }

  // add_3
  {
    auto a =
        make_histogram<array_storage<char>>(Type(), axis::integer<>(-1, 2));
    auto b =
        make_histogram<array_storage<unsigned>>(Type(), axis::integer<>(-1, 2));
    a.fill(-1);
    b.fill(1);
    auto c = a;
    c += b;
    BOOST_TEST_EQ(c.value(-1), 0);
    BOOST_TEST_EQ(c.value(0), 1);
    BOOST_TEST_EQ(c.value(1), 0);
    BOOST_TEST_EQ(c.value(2), 1);
    BOOST_TEST_EQ(c.value(3), 0);
    auto d = a;
    d += b;
    BOOST_TEST_EQ(d.value(-1), 0);
    BOOST_TEST_EQ(d.value(0), 1);
    BOOST_TEST_EQ(d.value(1), 0);
    BOOST_TEST_EQ(d.value(2), 1);
    BOOST_TEST_EQ(d.value(3), 0);
  }

  // bad_add
  {
    auto a = make_histogram<adaptive_storage>(Type(), axis::integer<>(0, 2));
    auto b = make_histogram<adaptive_storage>(Type(), axis::integer<>(0, 3));
    BOOST_TEST_THROWS(a += b, std::logic_error);
  }

  // bad_index
  {
    auto a = make_histogram<adaptive_storage>(Type(), axis::integer<>(0, 2));
    BOOST_TEST_THROWS(a.value(5), std::out_of_range);
    BOOST_TEST_THROWS(a.variance(5), std::out_of_range);
  }

  // functional programming
  {
    auto v = std::vector<int>{0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    auto h = make_histogram<adaptive_storage>(Type(), axis::integer<>(0, 10));
    std::for_each(v.begin(), v.end(), [&h](int x) { h.fill(weight(2.0), x); });
    BOOST_TEST_EQ(h.sum(), 20.0);
  }

  // operators
  {
    auto a = make_histogram<adaptive_storage>(Type(), axis::integer<>(0, 3));
    auto b = a;
    a.fill(0);
    b.fill(1);
    auto c = a + b;
    BOOST_TEST_EQ(c.value(0), 1);
    BOOST_TEST_EQ(c.value(1), 1);
    c += b;
    BOOST_TEST_EQ(c.value(0), 1);
    BOOST_TEST_EQ(c.value(1), 2);
    auto d = a + b + c;
    BOOST_TEST_EQ(d.value(0), 2);
    BOOST_TEST_EQ(d.value(1), 3);
    auto e = 3 * a;
    auto f = b * 2;
    BOOST_TEST_EQ(e.value(0), 3);
    BOOST_TEST_EQ(e.value(1), 0);
    BOOST_TEST_EQ(f.value(0), 0);
    BOOST_TEST_EQ(f.value(1), 2);
    auto r = a;
    r += b;
    r += e;
    BOOST_TEST_EQ(r.value(0), 4);
    BOOST_TEST_EQ(r.value(1), 1);
    BOOST_TEST_EQ(r, a + b + 3 * a);
    auto s = r / 4;
    r /= 4;
    BOOST_TEST_EQ(r.value(0), 1);
    BOOST_TEST_EQ(r.value(1), 0.25);
    BOOST_TEST_EQ(r, s);
  }

  // histogram_serialization
  {
    enum { A, B, C };
    auto a = make_histogram<adaptive_storage>(
        Type(), axis::regular<>(3, -1, 1, "r"),
        axis::circular<>(4, 0.0, 1.0, "p"),
        axis::regular<double, axis::transform::log>(3, 1, 100, "lr"),
        axis::variable<>({0.1, 0.2, 0.3, 0.4, 0.5}, "v"),
        axis::category<>{A, B, C}, axis::integer<>(0, 2, "i"));
    a.fill(0.5, 20, 0.1, 0.25, 1, 0);
    std::string buf;
    {
      std::ostringstream os;
      boost::archive::text_oarchive oa(os);
      oa << a;
      buf = os.str();
    }
    auto b = decltype(a)();
    BOOST_TEST_NE(a, b);
    {
      std::istringstream is(buf);
      boost::archive::text_iarchive ia(is);
      ia >> b;
    }
    BOOST_TEST_EQ(a, b);
  }

  // histogram_ostream
  {
    auto a = make_histogram<adaptive_storage>(
        Type(), axis::regular<>(3, -1, 1, "r"), axis::integer<>(0, 2, "i"));
    std::ostringstream os;
    os << a;
    BOOST_TEST_EQ(os.str(), "histogram("
                            "\n  regular(3, -1, 1, label='r'),"
                            "\n  integer(0, 2, label='i'),"
                            "\n)");
  }

  // histogram_reset
  {
    auto a = make_histogram<adaptive_storage>(
        Type(), axis::integer<>(0, 2, "", axis::uoflow::off));
    a.fill(0);
    a.fill(1);
    BOOST_TEST_EQ(a.value(0), 1);
    BOOST_TEST_EQ(a.value(1), 1);
    a.reset();
    BOOST_TEST_EQ(a.value(0), 0);
    BOOST_TEST_EQ(a.value(1), 0);
  }

  // reduce
  {
    auto h1 = make_histogram<adaptive_storage>(Type(), axis::integer<>(0, 2),
                                               axis::integer<>(0, 3));
    h1.fill(0, 0);
    h1.fill(0, 1);
    h1.fill(1, 0);
    h1.fill(1, 1);
    h1.fill(1, 2);

    auto h1_0 = h1.reduce_to(0_c);
    BOOST_TEST_EQ(h1_0.dim(), 1);
    BOOST_TEST_EQ(h1_0.sum(), 5);
    // BOOST_TEST_EQ(h1_0.value(0), 2);
    // BOOST_TEST_EQ(h1_0.value(1), 3);
    // BOOST_TEST_EQ(h1_0.axis()[0].lower(), 0.0);
    // BOOST_TEST_EQ(h1_0.axis()[1].lower(), 1.0);
    // BOOST_TEST(axis_equal(Type(), h1_0.axis(), h1.axis(0_c)));

    auto h1_1 = h1.reduce_to(1_c);
    BOOST_TEST_EQ(h1_1.dim(), 1);
    BOOST_TEST_EQ(h1_1.sum(), 5);
    BOOST_TEST_EQ(h1_1.value(0), 2);
    BOOST_TEST_EQ(h1_1.value(1), 2);
    BOOST_TEST_EQ(h1_1.value(2), 1);
    BOOST_TEST(axis_equal(Type(), h1_1.axis(), h1.axis(1_c)));

    auto h2 = make_histogram<adaptive_storage>(Type(), axis::integer<>(0, 2),
                                               axis::integer<>(0, 3),
                                               axis::integer<>(0, 4));
    h2.fill(0, 0, 0);
    h2.fill(0, 1, 0);
    h2.fill(0, 1, 1);
    h2.fill(0, 0, 2);
    h2.fill(1, 0, 2);

    auto h2_0 = h2.reduce_to(0_c);
    BOOST_TEST_EQ(h2_0.dim(), 1);
    BOOST_TEST_EQ(h2_0.sum(), 5);
    BOOST_TEST_EQ(h2_0.value(0), 4);
    BOOST_TEST_EQ(h2_0.value(1), 1);
    BOOST_TEST(axis_equal(Type(), h2_0.axis(), axis::integer<>(0, 2)));

    auto h2_1 = h2.reduce_to(1_c);
    BOOST_TEST_EQ(h2_1.dim(), 1);
    BOOST_TEST_EQ(h2_1.sum(), 5);
    BOOST_TEST_EQ(h2_1.value(0), 3);
    BOOST_TEST_EQ(h2_1.value(1), 2);
    BOOST_TEST(axis_equal(Type(), h2_1.axis(), axis::integer<>(0, 3)));

    auto h2_2 = h2.reduce_to(2_c);
    BOOST_TEST_EQ(h2_2.dim(), 1);
    BOOST_TEST_EQ(h2_2.sum(), 5);
    BOOST_TEST_EQ(h2_2.value(0), 2);
    BOOST_TEST_EQ(h2_2.value(1), 1);
    BOOST_TEST_EQ(h2_2.value(2), 2);
    BOOST_TEST(axis_equal(Type(), h2_2.axis(), axis::integer<>(0, 4)));

    auto h2_01 = h2.reduce_to(0_c, 1_c);
    BOOST_TEST_EQ(h2_01.dim(), 2);
    BOOST_TEST_EQ(h2_01.sum(), 5);
    BOOST_TEST_EQ(h2_01.value(0, 0), 2);
    BOOST_TEST_EQ(h2_01.value(0, 1), 2);
    BOOST_TEST_EQ(h2_01.value(1, 0), 1);
    BOOST_TEST(axis_equal(Type(), h2_01.axis(0_c), axis::integer<>(0, 2)));
    BOOST_TEST(axis_equal(Type(), h2_01.axis(1_c), axis::integer<>(0, 3)));

    auto h2_02 = h2.reduce_to(0_c, 2_c);
    BOOST_TEST_EQ(h2_02.dim(), 2);
    BOOST_TEST_EQ(h2_02.sum(), 5);
    BOOST_TEST_EQ(h2_02.value(0, 0), 2);
    BOOST_TEST_EQ(h2_02.value(0, 1), 1);
    BOOST_TEST_EQ(h2_02.value(0, 2), 1);
    BOOST_TEST_EQ(h2_02.value(1, 2), 1);
    BOOST_TEST(axis_equal(Type(), h2_02.axis(0_c), axis::integer<>(0, 2)));
    BOOST_TEST(axis_equal(Type(), h2_02.axis(1_c), axis::integer<>(0, 4)));

    auto h2_12 = h2.reduce_to(1_c, 2_c);
    BOOST_TEST_EQ(h2_12.dim(), 2);
    BOOST_TEST_EQ(h2_12.sum(), 5);
    BOOST_TEST_EQ(h2_12.value(0, 0), 1);
    BOOST_TEST_EQ(h2_12.value(1, 0), 1);
    BOOST_TEST_EQ(h2_12.value(1, 1), 1);
    BOOST_TEST_EQ(h2_12.value(0, 2), 2);
    BOOST_TEST(axis_equal(Type(), h2_12.axis(0_c), axis::integer<>(0, 3)));
    BOOST_TEST(axis_equal(Type(), h2_12.axis(1_c), axis::integer<>(0, 4)));
  }

  // custom axis
  {
    struct custom_axis : public axis::integer<> {
      using value_type = const char*; // type that is fed to the axis

      using integer::integer; // inherit ctors of base

      // the customization point
      // - accept const char* and convert to int
      // - then call index method of base class
      int index(value_type s) const {
        return integer::index(std::atoi(s));
      }
    };

    auto h = make_histogram<adaptive_storage>(Type(), custom_axis(0, 3));
    h.fill("-10");
    h.fill("0");
    h.fill("1");
    h.fill("9");

    BOOST_TEST_EQ(h.dim(), 1);
    BOOST_TEST(h.axis() == custom_axis(0, 3));
    BOOST_TEST_EQ(h.value(0), 1);
    BOOST_TEST_EQ(h.value(1), 1);
    BOOST_TEST_EQ(h.value(2), 0);
  }
}

template <typename T1, typename T2> void run_mixed_tests() {

  // compare
  {
    auto a = make_histogram<adaptive_storage>(T1{}, axis::regular<>{3, 0, 3},
                                              axis::integer<>(0, 2));
    auto b = make_histogram<array_storage<int>>(T2{}, axis::regular<>{3, 0, 3},
                                                axis::integer<>(0, 2));
    BOOST_TEST_EQ(a, b);
    auto b2 = make_histogram<adaptive_storage>(T2{}, axis::integer<>{0, 3},
                                               axis::integer<>(0, 2));
    BOOST_TEST_NE(a, b2);
    auto b3 = make_histogram<adaptive_storage>(T2{}, axis::regular<>(3, 0, 4),
                                               axis::integer<>(0, 2));
    BOOST_TEST_NE(a, b3);
  }

  // copy_assign
  {
    auto a = make_histogram<adaptive_storage>(T1{}, axis::regular<>{3, 0, 3},
                                              axis::integer<>(0, 2));
    auto b = make_histogram<array_storage<int>>(T2{}, axis::regular<>{3, 0, 3},
                                                axis::integer<>(0, 2));
    a.fill(1, 1);
    BOOST_TEST_NE(a, b);
    b = a;
    BOOST_TEST_EQ(a, b);
  }
}

int main() {

  // common interface
  run_tests<Static>();
  run_tests<Dynamic>();

  // special stuff that only works with Dynamic

  // init
  {
    auto v = std::vector<axis::any<>>();
    v.push_back(axis::regular<>(4, -1, 1));
    v.push_back(axis::integer<>(1, 7));
    auto h = make_dynamic_histogram(v.begin(), v.end());
    BOOST_TEST_EQ(h.axis(0), v[0]);
    BOOST_TEST_EQ(h.axis(1), v[1]);
  }

  // using iterator ranges
  {
    auto h = make_dynamic_histogram(axis::regular<>(2, -1, 1),
                                    axis::regular<>(2,  2, 4));
    auto v = std::vector<double>(2);
    v = {-0.5, 2.5};
    h.fill(v.begin(), v.end());
    v = { 0.5, 3.5};
    h.fill(v.begin(), v.end());
    auto i = std::vector<int>(2);
    i = {0, 0};
    BOOST_TEST_EQ(h.value(i.begin(), i.end()), 1);
    i = {1, 1};
    BOOST_TEST_EQ(h.variance(i.begin(), i.end()), 1);
  }

  // axis methods
  {
    enum { A, B };
    auto c = make_dynamic_histogram(axis::category<>({A, B}));
    BOOST_TEST_THROWS(c.axis()[0].lower(), std::runtime_error);
    BOOST_TEST_THROWS(c.axis()[0].upper(), std::runtime_error);
  }

  // reduce
  {
    auto h1 =
        make_dynamic_histogram(axis::integer<>(0, 2), axis::integer<>(0, 3));
    h1.fill(0, 0);
    h1.fill(0, 1);
    h1.fill(1, 0);
    h1.fill(1, 1);
    h1.fill(1, 2);

    auto h1_0 = h1.reduce_to(0);
    BOOST_TEST_EQ(h1_0.dim(), 1);
    BOOST_TEST_EQ(h1_0.sum(), 5);
    BOOST_TEST_EQ(h1_0.value(0), 2);
    BOOST_TEST_EQ(h1_0.value(1), 3);
    BOOST_TEST(axis_equal(Dynamic(), h1_0.axis(), h1.axis(0_c)));

    auto h1_1 = h1.reduce_to(1);
    BOOST_TEST_EQ(h1_1.dim(), 1);
    BOOST_TEST_EQ(h1_1.sum(), 5);
    BOOST_TEST_EQ(h1_1.value(0), 2);
    BOOST_TEST_EQ(h1_1.value(1), 2);
    BOOST_TEST_EQ(h1_1.value(2), 1);
    BOOST_TEST(axis_equal(Dynamic(), h1_1.axis(), h1.axis(1_c)));
  }

  run_mixed_tests<Static, Dynamic>();
  run_mixed_tests<Dynamic, Static>();

  return boost::report_errors();
}
