# Histogram

**Fast multi-dimensional histogram with convenient interface for C++11 and Python**

[![Build Status](https://travis-ci.org/HDembinski/histogram.svg?branch=master)](https://travis-ci.org/HDembinski/histogram?branch=master) [![Coverage Status](https://coveralls.io/repos/github/HDembinski/histogram/badge.svg?branch=master)](https://coveralls.io/github/HDembinski/histogram?branch=master)

This `C++11` library provides a multi-dimensional [histogram](https://en.wikipedia.org/wiki/Histogram) class for your statistics needs. It is very customisable through policy classes, but the default policies were carefully designed so that most users won't need to customize anything. In the standard configuration, this library offers a unique safety guarantee not found elsewhere: bin counts *cannot overflow* or *be capped*. While being safe to use, the library also has a convenient interface, is memory conserving, and faster than other libraries (see benchmarks).

The histogram class comes in two variants which share a common interface. The *static* variant uses compile-time information to provide maximum performance, at the cost of runtime flexibility and potentially larger executables. The *dynamic* variant is a bit slower, but configurable at run-time and may produce smaller executables. Python bindings for the latter are included, implemented with `Boost.Python`.

The histogram supports value semantics. Histograms can be added and scaled. Move operations and trips over the language boundary from C++ to Python and back are cheap. Histogram instances can be streamed from/to files and pickled in Python. [Numpy](http://www.numpy.org) is supported to speed up operations in Python: histograms can be filled with Numpy arrays at high speed (in most cases several times faster than numpy's own histogram function) and are convertible into Numpy array views without copying data.

My goal is to submit this project to [Boost](http://www.boost.org), that's why it uses the Boost directory structure and namespace. The code is released under the [Boost Software License](http://www.boost.org/LICENSE_1_0.txt).

Check out the [full documentation](https://htmlpreview.github.io/?https://raw.githubusercontent.com/HDembinski/histogram/html/doc/html/index.html). Highlights are given below.

## Features

* Multi-dimensional histogram
* Simple and convenient interface in C++ and Python
* Static and dynamic implementation in C++ with common interface
* Counters cannot overflow or be capped (+)
* Better performance than other libraries (see benchmarks for details)
* Efficient move operations
* Efficient conversion between static and dynamic implementation
* Efficient use of memory (counter capacity dynamically grows as needed)
* Support for many mappings of input values to bin indices (user extensible)
* Support for weighted increments
* Support for under-/overflow bins (can be disabled individually for each dimension)
* Support for variance tracking (++)
* Support for addition and scaling of histograms
* Support for serialization based on `Boost.Serialization`
* Support for Python 2.x and 3.x
* Support for Numpy

(+) In the standard configuration and if you don't use weighted input.
(++) If you don't use weighted increments, variance tracking come at zero cost. If you use weighted increments, extra space is reserved internally to keep track of a variance counter per bin. The conversion happens automatically and transparently.

## Dependencies

* [Boost](http://www.boost.org)
* Optional: [CMake](https://cmake.org) [Python](http://www.python.org) [Numpy](http://www.numpy.org)

## Build instructions

The library can be build with `b2` within the boost directory structure, but if you are not a boost developer, use `cmake` instead.

```sh
git clone https://github.com/HDembinski/histogram.git
mkdir build && cd build
cmake ../histogram/build
make # or 'make install'
```

To run the tests, do `make test`.

## Code examples

For the full version of the following examples with explanations, see
the [Getting started](https://htmlpreview.github.io/?https://raw.githubusercontent.com/HDembinski/histogram/html/doc/html/getting_started.html) section in the documentation.

Example 1: Fill a 1d-histogram in C++

```cpp
    #include <boost/histogram.hpp> // proposed for inclusion in Boost
    #include <iostream>
    #include <cmath>

    int main(int, char**) {
        namespace bh = boost::histogram;
        using namespace boost::histogram::literals; // enables _c suffix

        // create 1d-histogram with 10 equidistant bins from -1.0 to 2.0,
        // with axis of histogram labeled as "x"
        auto h = bh::make_static_histogram(bh::axis::regular<>(10, -1.0, 2.0, "x"));

        // fill histogram with data
        h.fill(-1.5); // put in underflow bin
        h.fill(-1.0); // included in first bin, bin interval is semi-open
        h.fill(-0.5);
        h.fill(1.1);
        h.fill(0.3);
        h.fill(1.7);
        h.fill(2.0);  // put in overflow bin, bin interval is semi-open
        h.fill(20.0); // put in overflow bin
        h.fill(0.1, bh::weight(5)); // fill with a weighted entry, weight is 5

        // iterate over bins, loop skips under- and overflow bin
        for (const auto& bin : h.axis(0_c)) {
            std::cout << "bin " << bin.first
                      << " x in [" << bin.second.lower() << ", " << bin.second.upper() << "): "
                      << h.value(bin.first) << " +/- " << std::sqrt(h.variance(bin.first))
                      << std::endl;
        }

        /* program output:

        bin 0 x in [-1, -0.7): 1 +/- 1
        bin 1 x in [-0.7, -0.4): 1 +/- 1
        bin 2 x in [-0.4, -0.1): 0 +/- 0
        bin 3 x in [-0.1, 0.2): 5 +/- 5
        bin 4 x in [0.2, 0.5): 1 +/- 1
        bin 5 x in [0.5, 0.8): 0 +/- 0
        bin 6 x in [0.8, 1.1): 0 +/- 0
        bin 7 x in [1.1, 1.4): 1 +/- 1
        bin 8 x in [1.4, 1.7): 0 +/- 0
        bin 9 x in [1.7, 2): 1 +/- 1

        */
    }
```

Example 2: Fill a 2d-histogram in Python with data in Numpy arrays

```python
    import histogram as bh
    import numpy as np

    # create 2d-histogram over polar coordinates, with
    # 10 equidistant bins in radius from 0 to 5 and
    # 4 equidistant bins in polar angle
    h = bh.histogram(bh.axis.regular(10, 0.0, 5.0, "radius", uoflow=False),
                     bh.axis.circular(4, 0.0, 2*np.pi, "phi"))

    # generate some numpy arrays with data to fill into histogram,
    # in this case normal distributed random numbers in x and y,
    # converted into polar coordinates
    x = np.random.randn(1000)             # generate x
    y = np.random.randn(1000)             # generate y
    radius = (x ** 2 + y ** 2) ** 0.5
    phi = np.arctan2(y, x)

    # fill histogram with numpy arrays; the call looks the
    # if radius and phi are numbers instead of arrays
    h.fill(radius, phi)

    # access histogram counts (no copy)
    count_matrix = np.asarray(h)

    print count_matrix

    # program output:
    #
    # [[37 26 33 37]
    #  [60 69 76 62]
    #  [48 80 80 77]
    #  [38 49 45 49]
    #  [22 24 20 23]
    #  [ 7  9  9  8]
    #  [ 3  2  3  3]
    #  [ 0  0  0  0]
    #  [ 0  1  0  0]
    #  [ 0  0  0  0]]
```

## Benchmarks

Thanks to meta-programming and dynamic memory management, this library is not only safer, more flexible and convenient to use, but also faster than the competition. In the plot below, its speed is compared to classes from the [GNU Scientific Library](https://www.gnu.org/software/gsl), the [ROOT framework from CERN](https://root.cern.ch), and to the histogram functions in [Numpy](http://www.numpy.org). The orange to red items are different compile-time configurations of the histogram in this library. More details on the benchmark are given in the [documentation](https://htmlpreview.github.io/?https://raw.githubusercontent.com/HDembinski/histogram/html/doc/html/histogram/benchmarks.html)

![alt benchmark](doc/benchmark.png)

## Rationale

There is a lack of a widely-used free histogram class in C++. While it is easy to write a one-dimensional histogram, writing a general multi-dimensional histogram is not trivial. Even more so, if you want the histogram to be serializable and have Python-bindings and support Numpy. In high-energy physics, the [ROOT framework](https://root.cern.ch) from CERN is widely used. This histogram class is designed to be more convenient, more flexiable, and faster than the equivalent ROOT histograms. This library comes in a clean and modern C++ design which follows the advice given in popular C++ books, like those of [Meyers](http://www.aristeia.com/books.html) and [Sutter and Alexandrescu](http://www.gotw.ca/publications/c++cs.htm).

Read more about the design choices in the [documentation](https://htmlpreview.github.io/?https://raw.githubusercontent.com/HDembinski/histogram/html/doc/html/histogram/rationale.html)

## State of project

The histogram is feature-complete. More than 500 individual tests make sure that the implementation works as expected. Full documentation is available. User feedback is appreciated!
