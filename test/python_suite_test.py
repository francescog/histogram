# -*- coding: utf-8 -*-
##
## Copyright 2015-2016 Hans Dembinski
##
## Distributed under the Boost Software License, Version 1.0.
## (See accompanying file LICENSE_1_0.txt
## or copy at http://www.boost.org/LICENSE_1_0.txt)

import unittest
from math import pi
from histogram import histogram
from histogram.axis import (regular, regular_log, regular_sqrt, regular_cos,
                            regular_pow, circular, variable, category,
                            integer)
import pickle
import os
import sys
if sys.version_info.major == 3:
    from io import BytesIO
else:
    from StringIO import StringIO as BytesIO

ON = True
OFF = False
have_numpy = @BUILD_NUMPY_SUPPORT@
if have_numpy:
    import numpy

class test_regular(unittest.TestCase):

    def test_init(self):
        regular(1, 1.0, 2.0)
        regular(1, 1.0, 2.0, label="ra")
        regular(1, 1.0, 2.0, uoflow=False)
        regular(1, 1.0, 2.0, label="ra", uoflow=False)
        regular_log(1, 1.0, 2.0)
        regular_sqrt(1, 1.0, 2.0)
        regular_cos(1, 0.5, 1.0)
        regular_pow(1, 1.0, 2.0, 1.5)
        with self.assertRaises(TypeError):
            regular()
        with self.assertRaises(TypeError):
            regular(1)
        with self.assertRaises(TypeError):
            regular(1, 1.0)
        with self.assertRaises(RuntimeError):
            regular(0, 1.0, 2.0)
        with self.assertRaises(TypeError):
            regular("1", 1.0, 2.0)
        with self.assertRaises(Exception):
            regular(-1, 1.0, 2.0)
        with self.assertRaises(RuntimeError):
            regular(1, 2.0, 1.0)
        with self.assertRaises(TypeError):
            regular(1, 1.0, 2.0, label=0)
        with self.assertRaises(TypeError):
            regular(1, 1.0, 2.0, label="ra", uoflow="True")
        with self.assertRaises(TypeError):
            regular(1, 1.0, 2.0, bad_keyword="ra")
        with self.assertRaises(TypeError):
            regular_pow(1, 1.0, 2.0)
        a = regular(4, 1.0, 2.0)
        self.assertEqual(a, regular(4, 1.0, 2.0))
        self.assertNotEqual(a, regular(3, 1.0, 2.0))
        self.assertNotEqual(a, regular(4, 1.1, 2.0))
        self.assertNotEqual(a, regular(4, 1.0, 2.1))

    def test_len(self):
        a = regular(4, 1.0, 2.0)
        self.assertEqual(len(a), 4)

    def test_repr(self):
        for s in ("regular(4, 1.1, 2.2)",
                  "regular(4, 1.1, 2.2, label='ra')",
                  "regular(4, 1.1, 2.2, uoflow=False)",
                  "regular(4, 1.1, 2.2, label='ra', uoflow=False)",
                  "regular_log(4, 1.1, 2.2)",
                  "regular_pow(4, 1.1, 2.2, 0.5)"):
            self.assertEqual(str(eval(s)), s)

    def test_getitem(self):
        v = [1.0, 1.25, 1.5, 1.75, 2.0]
        a = regular(4, 1.0, 2.0)
        for i in range(4):
            self.assertAlmostEqual(a[i][0], v[i])
            self.assertAlmostEqual(a[i][1], v[i+1])
        self.assertEqual(a[-1][0], -float("infinity"))
        self.assertEqual(a[4][1], float("infinity"))

    def test_iter(self):
        v = [1.0, 1.25, 1.5, 1.75, 2.0]
        a = regular(4, 1.0, 2.0)
        self.assertAlmostEqual([x[0] for x in a], v[:-1])
        self.assertAlmostEqual([x[1] for x in a], v[1:])

    def test_index(self):
        a = regular(4, 1.0, 2.0)
        self.assertEqual(a.index(-1), -1)
        self.assertEqual(a.index(0.99), -1)
        self.assertEqual(a.index(1.0), 0)
        self.assertEqual(a.index(1.249), 0)
        self.assertEqual(a.index(1.250), 1)
        self.assertEqual(a.index(1.499), 1)
        self.assertEqual(a.index(1.500), 2)
        self.assertEqual(a.index(1.749), 2)
        self.assertEqual(a.index(1.750), 3)
        self.assertEqual(a.index(1.999), 3)
        self.assertEqual(a.index(2.000), 4)
        self.assertEqual(a.index(20), 4)

    def test_log_transform(self):
        a = regular_log(2, 1e0, 1e2)
        self.assertEqual(a.index(-1), -1)
        self.assertEqual(a.index(0.99), -1)
        self.assertEqual(a.index(1.0), 0)
        self.assertEqual(a.index(9.99), 0)
        self.assertEqual(a.index(10.0), 1)
        self.assertEqual(a.index(99.9), 1)
        self.assertEqual(a.index(100), 2)
        self.assertEqual(a.index(1000), 2)
        self.assertAlmostEqual(a[0][0], 1e0)
        self.assertAlmostEqual(a[1][0], 1e1)
        self.assertAlmostEqual(a[1][1], 1e2)

    def test_pow_transform(self):
        a = regular_pow(2, 1.0, 9.0, 0.5)
        self.assertEqual(a.index(-1), -1)
        self.assertEqual(a.index(0.99), -1)
        self.assertEqual(a.index(1.0), 0)
        self.assertEqual(a.index(3.99), 0)
        self.assertEqual(a.index(4.0), 1)
        self.assertEqual(a.index(8.99), 1)
        self.assertEqual(a.index(9), 2)
        self.assertEqual(a.index(1000), 2)
        self.assertAlmostEqual(a[0][0], 1.0)
        self.assertAlmostEqual(a[1][0], 4.0)
        self.assertAlmostEqual(a[1][1], 9.0)

class test_circular(unittest.TestCase):

    def test_init(self):
        circular(1)
        circular(4, 1.0)
        circular(4, 1.0, label="pa")
        with self.assertRaises(TypeError):
            circular()
        with self.assertRaises(Exception):
            circular(-1)
        with self.assertRaises(TypeError):
            circular(4, 1.0, uoflow=True)
        with self.assertRaises(TypeError):
            circular(1, 1.0, 2.0, 3.0)
        with self.assertRaises(TypeError):
            circular(1, 1.0, label=1)
        with self.assertRaises(TypeError):
            circular("1")
        a = circular(4, 1.0)
        self.assertEqual(a, circular(4, 1.0))
        self.assertNotEqual(a, circular(2, 1.0))
        self.assertNotEqual(a, circular(4, 0.0))

    def test_len(self):
        self.assertEqual(len(circular(4)), 4)
        self.assertEqual(len(circular(4, 1.0)), 4)

    def test_repr(self):
        for s in ("circular(4)",
                  "circular(4, phase=1)",
                  "circular(4, phase=1, label='x')",
                  "circular(4, label='x')"):
            self.assertEqual(str(eval(s)), s)

    def test_getitem(self):
        v = [1.0, 1.0 + 0.5 * pi, 1.0 + pi, 1.0 + 1.5 *pi, 1.0 + 2.0 * pi]
        a = circular(4, 1.0)
        for i in range(4):
            self.assertEqual(a[i][0], v[i])
            self.assertEqual(a[i][1], v[i+1])

    def test_iter(self):
        a = circular(4, 1.0)
        v = [1.0, 1.0 + 0.5 * pi, 1.0 + pi, 1.0 + 1.5 *pi, 1.0 + 2.0 * pi]
        self.assertEqual([x[0] for x in a], v[:-1])
        self.assertEqual([x[1] for x in a], v[1:])

    def test_index(self):
        a = circular(4, 1.0)
        d = 0.5 * pi
        self.assertEqual(a.index(0.99 - 4*d), 3)
        self.assertEqual(a.index(0.99 - 3*d), 0)
        self.assertEqual(a.index(0.99 - 2*d), 1)
        self.assertEqual(a.index(0.99 - d), 2)
        self.assertEqual(a.index(0.99), 3)
        self.assertEqual(a.index(1.0), 0)
        self.assertEqual(a.index(1.01), 0)
        self.assertEqual(a.index(0.99 + d), 0)
        self.assertEqual(a.index(1.0 + d), 1)
        self.assertEqual(a.index(1.0 + 2*d), 2)
        self.assertEqual(a.index(1.0 + 3*d), 3)
        self.assertEqual(a.index(1.0 + 4*d), 0)
        self.assertEqual(a.index(1.0 + 5*d), 1)

class test_variable(unittest.TestCase):

    def test_init(self):
        variable(0, 1)
        variable(1, -1)
        variable(0, 1, 2, 3, 4)
        variable(0, 1, label="va")
        variable(0, 1, uoflow=True)
        variable(0, 1, label="va", uoflow=True)
        with self.assertRaises(TypeError):
            variable()
        with self.assertRaises(RuntimeError):
            variable(1.0)
        with self.assertRaises(TypeError):
            variable("1", 2)
        with self.assertRaises(KeyError):
            variable(0.0, 1.0, 2.0, bad_keyword="ra")
        a = variable(-0.1, 0.2, 0.3)
        self.assertEqual(a, variable(-0.1, 0.2, 0.3))
        self.assertNotEqual(a, variable(0, 0.2, 0.3))
        self.assertNotEqual(a, variable(-0.1, 0.1, 0.3))
        self.assertNotEqual(a, variable(-0.1, 0.1))

    def test_len(self):
        self.assertEqual(len(variable(-0.1, 0.2, 0.3)), 2)

    def test_repr(self):
        for s in ("variable(-0.1, 0.2)",
                  "variable(-0.1, 0.2, 0.3)",
                  "variable(-0.1, 0.2, 0.3, label='va')",
                  "variable(-0.1, 0.2, 0.3, uoflow=False)",
                  "variable(-0.1, 0.2, 0.3, label='va', uoflow=False)"):
            self.assertEqual(str(eval(s)), s)

    def test_getitem(self):
        v = [-0.1, 0.2, 0.3]
        a = variable(*v)
        for i in range(2):
            self.assertEqual(a[i][0], v[i])
            self.assertEqual(a[i][1], v[i+1])
        self.assertEqual(a[-1][0], -float("infinity"))
        self.assertEqual(a[2][1], float("infinity"))

    def test_iter(self):
        v = [-0.1, 0.2, 0.3]
        a = variable(*v)
        self.assertEqual([x[0] for x in a], v[:-1])
        self.assertEqual([x[1] for x in a], v[1:])

    def test_index(self):
        a = variable(-0.1, 0.2, 0.3)
        self.assertEqual(a.index(-10.0), -1)
        self.assertEqual(a.index(-0.11), -1)
        self.assertEqual(a.index(-0.1), 0)
        self.assertEqual(a.index(0.0), 0)
        self.assertEqual(a.index(0.19), 0)
        self.assertEqual(a.index(0.2), 1)
        self.assertEqual(a.index(0.21), 1)
        self.assertEqual(a.index(0.29), 1)
        self.assertEqual(a.index(0.3), 2)
        self.assertEqual(a.index(0.31), 2)
        self.assertEqual(a.index(10), 2)

class test_integer(unittest.TestCase):

    def test_init(self):
        integer(-1, 2)
        with self.assertRaises(TypeError):
            integer()
        with self.assertRaises(TypeError):
            integer(1)
        with self.assertRaises(TypeError):
            integer("1", 2)
        with self.assertRaises(RuntimeError):
            integer(2, -1)
        with self.assertRaises(TypeError):
            integer(1, 2, 3)
        self.assertEqual(integer(-1, 2), integer(-1, 2))
        self.assertNotEqual(integer(-1, 2), integer(-1, 2, label="ia"))
        self.assertNotEqual(integer(-1, 2, uoflow=False),
                            integer(-1, 2, uoflow=True))

    def test_len(self):
        self.assertEqual(len(integer(-1, 3)), 4)

    def test_repr(self):
        for s in ("integer(-1, 1)",
                  "integer(-1, 1, label='ia')",
                  "integer(-1, 1, uoflow=False)",
                  "integer(-1, 1, label='ia', uoflow=False)"):
            self.assertEqual(str(eval(s)), s)

    def test_label(self):
        self.assertEqual(integer(-1, 2, label="ia").label, "ia")

    def test_getitem(self):
        v = [-1, 0, 1, 2]
        a = integer(-1, 3)
        for i in range(4):
            self.assertEqual(a[i][0], v[i])
        self.assertEqual(a[-1][0], -2 ** 31 + 1)
        self.assertEqual(a[4][1], 2 ** 31 - 1)

    def test_iter(self):
        v = [-1, 0, 1, 2, 3]
        a = integer(-1, 3)
        self.assertEqual([x[0] for x in a], v[:-1])
        self.assertEqual([x[1] for x in a], v[1:])

    def test_index(self):
        a = integer(-1, 3)
        self.assertEqual(a.index(-3), -1)
        self.assertEqual(a.index(-2), -1)
        self.assertEqual(a.index(-1), 0)
        self.assertEqual(a.index(0), 1)
        self.assertEqual(a.index(1), 2)
        self.assertEqual(a.index(2), 3)
        self.assertEqual(a.index(3), 4)
        self.assertEqual(a.index(4), 4)

class test_category(unittest.TestCase):

    def test_init(self):
        category(1, 2, 3)
        category(1, 2, 3, label="ca")
        with self.assertRaises(TypeError):
            category()
        with self.assertRaises(TypeError):
            category("1")
        with self.assertRaises(TypeError):
            category(1, "2")
        with self.assertRaises(TypeError):
            category(1, 2, label=1)
        with self.assertRaises(KeyError):
            category(1, 2, 3, uoflow=True)
        self.assertEqual(category(1, 2, 3),
                         category(1, 2, 3))

    def test_len(self):
        a = category(1, 2, 3)
        self.assertEqual(len(a), 3)

    def test_repr(self):
        for s in ("category(1)",
                  "category(1, 2)",
                  "category(1, 2, 3)"):
            self.assertEqual(str(eval(s)), s)

    def test_getitem(self):
        c = 1, 2, 3
        a = category(*c)
        for i in range(3):
            self.assertEqual(a[i], c[i])

    def test_iter(self):
        c = [1, 2, 3]
        self.assertEqual([x for x in category(*c)], c)

class test_histogram(unittest.TestCase):

    def test_init(self):
        histogram()
        histogram(integer(-1, 1))
        with self.assertRaises(TypeError):
            histogram(1)
        with self.assertRaises(TypeError):
            histogram("bla")
        with self.assertRaises(TypeError):
            histogram([])
        with self.assertRaises(TypeError):
            histogram(regular)
        with self.assertRaises(TypeError):
            histogram(regular())
        with self.assertRaises(TypeError):
            histogram([integer(-1, 1)])
        with self.assertRaises(RuntimeError):
            histogram(integer(-1, 1), unknown_keyword="nh")

        h = histogram(integer(-1, 2))
        self.assertEqual(h.dim, 1)
        self.assertEqual(h.axis(0), integer(-1, 2))
        self.assertEqual(h.axis(0).shape, 5)
        self.assertEqual(histogram(integer(-1, 2, uoflow=False)).axis(0).shape, 3)
        self.assertNotEqual(h, histogram(regular(1, -1, 1)))
        self.assertNotEqual(h, histogram(integer(-1, 2)))
        self.assertNotEqual(h, histogram(integer(-1, 1, label="ia")))

    def test_copy(self):
        a = histogram(integer(-1, 1))
        import copy
        b = copy.copy(a)
        self.assertEqual(a, b)
        self.assertNotEqual(id(a), id(b))
        c = copy.deepcopy(b)
        self.assertEqual(b, c)
        self.assertNotEqual(id(b), id(c))

    def test_fill_1d(self):
        h0 = histogram(integer(-1, 2, uoflow=False))
        h1 = histogram(integer(-1, 2, uoflow=True))
        for h in (h0, h1):
            with self.assertRaises(ValueError):
                h.fill()
            with self.assertRaises(ValueError):
                h.fill(1, 2)
            h.fill(-10)
            h.fill(-1)
            h.fill(-1)
            h.fill(0)
            h.fill(1)
            h.fill(1)
            h.fill(1)
            h.fill(10)
        self.assertEqual(h0.sum, 6)
        self.assertEqual(h0.axis(0).shape, 3)
        self.assertEqual(h1.sum, 8)
        self.assertEqual(h1.axis(0).shape, 5)

        for h in (h0, h1):
            self.assertEqual(h.value(0), 2)
            self.assertEqual(h.value(1), 1)
            self.assertEqual(h.value(2), 3)
            with self.assertRaises(RuntimeError):
                h.value(0, 1)
            with self.assertRaises(RuntimeError):
                h.value(0, foo=None)
            self.assertEqual(h.variance(0), 2)
            self.assertEqual(h.variance(1), 1)
            self.assertEqual(h.variance(2), 3)
            with self.assertRaises(RuntimeError):
                h.variance(0, 1)
            with self.assertRaises(RuntimeError):
                h.variance(0, foo=None)

        self.assertEqual(h1.value(-1), 1)
        self.assertEqual(h1.value(3), 1)

    def test_growth(self):
        h = histogram(integer(-1, 2))
        h.fill(-1)
        h.fill(1)
        h.fill(1)
        for i in range(255):
            h.fill(0)
        h.fill(0)
        for i in range(1000-256):
            h.fill(0)
        self.assertEqual(h.value(-1), 0)
        self.assertEqual(h.value(0), 1)
        self.assertEqual(h.value(1), 1000)
        self.assertEqual(h.value(2), 2)
        self.assertEqual(h.value(3), 0)

    def test_fill_2d(self):
        for uoflow in (False, True):
            h = histogram(integer(-1, 2, uoflow=uoflow),
                           regular(4, -2, 2, uoflow=uoflow))
            h.fill(-1, -2)
            h.fill(-1, -1)
            h.fill(0, 0)
            h.fill(0, 1)
            h.fill(1, 0)
            h.fill(3, -1)
            h.fill(0, -3)
            with self.assertRaises(Exception):
                h.fill(1)
            with self.assertRaises(Exception):
                h.fill(1, 2, 3)

            m = [[1, 1, 0, 0, 0, 0],
                 [0, 0, 1, 1, 0, 1],
                 [0, 0, 1, 0, 0, 0],
                 [0, 1, 0, 0, 0, 0],
                 [0, 0, 0, 0, 0, 0]]
            for i in range(-uoflow, len(h.axis(0)) + uoflow):
                for j in range(-uoflow, len(h.axis(1)) + uoflow):
                    self.assertEqual(h.value(i, j), m[i][j])

    def test_add_2d(self):
        for uoflow in (False, True):
            h = histogram(integer(-1, 2, uoflow=uoflow),
                          regular(4, -2, 2, uoflow=uoflow))
            h.fill(-1, -2)
            h.fill(-1, -1)
            h.fill(0, 0)
            h.fill(0, 1)
            h.fill(1, 0)
            h.fill(3, -1)
            h.fill(0, -3)

            m = [[1, 1, 0, 0, 0, 0],
                 [0, 0, 1, 1, 0, 1],
                 [0, 0, 1, 0, 0, 0],
                 [0, 1, 0, 0, 0, 0],
                 [0, 0, 0, 0, 0, 0]]

            h += h

            for i in range(-uoflow, len(h.axis(0)) + uoflow):
                for j in range(-uoflow, len(h.axis(1)) + uoflow):
                    self.assertEqual(h.value(i, j), 2 * m[i][j])
                    self.assertEqual(h.variance(i, j), 2 * m[i][j])

    def test_add_2d_bad(self):
        a = histogram(integer(-1, 1))
        b = histogram(regular(3, -1, 1))
        with self.assertRaises(RuntimeError):
            a += b

    def test_add_2d_w(self):
        for uoflow in (False, True):
            h = histogram(integer(-1, 2, uoflow=uoflow),
                          regular(4, -2, 2, uoflow=uoflow))
            h.fill(-1, -2)
            h.fill(-1, -1)
            h.fill(0, 0)
            h.fill(0, 1)
            h.fill(1, 0)
            h.fill(3, -1)
            h.fill(0, -3)

            m = [[1, 1, 0, 0, 0, 0],
                 [0, 0, 1, 1, 0, 1],
                 [0, 0, 1, 0, 0, 0],
                 [0, 1, 0, 0, 0, 0],
                 [0, 0, 0, 0, 0, 0]]

            h2 = histogram(integer(-1, 2, uoflow=uoflow),
                           regular(4, -2, 2, uoflow=uoflow))
            h2.fill(0, 0, weight=0)

            h2 += h
            h2 += h
            h += h
            self.assertEqual(h, h2)

            for i in range(-uoflow, len(h.axis(0)) + uoflow):
                for j in range(-uoflow, len(h.axis(1)) + uoflow):
                    self.assertEqual(h.value(i, j), 2 * m[i][j])
                    self.assertEqual(h.variance(i, j), 2 * m[i][j])

    def test_repr(self):
        h = histogram(regular(10, 0, 1), integer(0, 1))
        h2 = eval(repr(h))
        self.assertEqual(h, h2)

    def test(self):
        axes = (regular(10, 0, 1), integer(0, 1))
        h = histogram(*axes)
        for i, a in enumerate(axes):
            self.assertEqual(h.axis(i), a)
        with self.assertRaises(IndexError):
            h.axis(2)
        self.assertEqual(h.axis(-1), axes[-1])
        self.assertEqual(h.axis(-2), axes[-2])
        with self.assertRaises(IndexError):
            h.axis(-3)

    def test_overflow(self):
        h = histogram(*[regular(1, 0, 1) for i in range(50)])
        with self.assertRaises(RuntimeError):
            h.fill(*range(50))

        with self.assertRaises(RuntimeError):
            h.value(*range(50))

        with self.assertRaises(RuntimeError):
            h.variance(*range(50))

    def test_out_of_range(self):
        h = histogram(regular(3, 0, 1))
        h.fill(-1)
        h.fill(2)
        self.assertEqual(h.value(-1), 1)
        self.assertEqual(h.value(3), 1)
        with self.assertRaises(IndexError):
            h.value(-2)
        with self.assertRaises(IndexError):
            h.value(4)
        with self.assertRaises(IndexError):
            h.variance(-2)
        with self.assertRaises(IndexError):
            h.variance(4)

    def test_operators(self):
        h = histogram(integer(0, 2))
        h.fill(0)
        h += h
        self.assertEqual(h.value(0), 2)
        self.assertEqual(h.variance(0), 2)
        self.assertEqual(h.value(1), 0)
        h *= 2
        self.assertEqual(h.value(0), 4)
        self.assertEqual(h.variance(0), 8)
        self.assertEqual(h.value(1), 0)
        self.assertEqual((h + h).value(0), (2 * h).value(0))
        self.assertEqual((h + h).value(0), (h * 2).value(0))
        self.assertNotEqual((h + h).variance(0), (2 * h).variance(0))
        self.assertNotEqual((h + h).variance(0), (h * 2).variance(0))
        h2 = histogram(regular(2, 0, 2))
        with self.assertRaises(RuntimeError):
            h + h2

    def test_reduce_to(self):
        h = histogram(integer(0, 2), integer(1, 4))
        h.fill(0, 1)
        h.fill(0, 2)
        h.fill(1, 3)

        h0 = h.reduce_to(0)
        self.assertEqual(h0.dim, 1)
        self.assertEqual(h0.axis(), integer(0, 2))
        self.assertEqual([h0.value(i) for i in range(2)], [2, 1])

        h1 = h.reduce_to(1)
        self.assertEqual(h1.dim, 1)
        self.assertEqual(h1.axis(), integer(1, 4))
        self.assertEqual([h1.value(i) for i in range(3)], [1, 1, 1])

    def test_pickle_0(self):
        a = histogram(category(0, 1, 2),
                      integer(0, 20, label='ia'),
                      regular(20, 0.0, 20.0, uoflow=False),
                      variable(0.0, 1.0, 2.0),
                      circular(4, label='pa'))
        for i in range(len(a.axis(0))):
            a.fill(i, 0, 0, 0, 0)
            for j in range(len(a.axis(1))):
                a.fill(i, j, 0, 0, 0)
                for k in range(len(a.axis(2))):
                    a.fill(i, j, k, 0, 0)
                    for l in range(len(a.axis(3))):
                        a.fill(i, j, k, l, 0)
                        for m in range(len(a.axis(4))):
                            a.fill(i, j, k, l, m * 0.5 * pi)

        io = BytesIO()
        pickle.dump(a, io)
        io.seek(0)
        b = pickle.load(io)
        self.assertNotEqual(id(a), id(b))
        self.assertEqual(a.dim, b.dim)
        self.assertEqual(a.axis(0), b.axis(0))
        self.assertEqual(a.axis(1), b.axis(1))
        self.assertEqual(a.axis(2), b.axis(2))
        self.assertEqual(a.axis(3), b.axis(3))
        self.assertEqual(a.axis(4), b.axis(4))
        self.assertEqual(a.sum, b.sum)
        self.assertEqual(a, b)

    def test_pickle_1(self):
        a = histogram(category(0, 1, 2),
                       integer(0, 3, label='ia'),
                       regular(4, 0.0, 4.0, uoflow=False),
                       variable(0.0, 1.0, 2.0))
        for i in range(len(a.axis(0))):
            a.fill(i, 0, 0, 0, weight=3)
            for j in range(len(a.axis(1))):
                a.fill(i, j, 0, 0, weight=10)
                for k in range(len(a.axis(2))):
                    a.fill(i, j, k, 0, weight=2)
                    for l in range(len(a.axis(3))):
                        a.fill(i, j, k, l, weight=5)

        io = BytesIO()
        pickle.dump(a, io)
        io.seek(0)
        b = pickle.load(io)
        self.assertNotEqual(id(a), id(b))
        self.assertEqual(a.dim, b.dim)
        self.assertEqual(a.axis(0), b.axis(0))
        self.assertEqual(a.axis(1), b.axis(1))
        self.assertEqual(a.axis(2), b.axis(2))
        self.assertEqual(a.axis(3), b.axis(3))
        self.assertEqual(a.sum, b.sum)
        self.assertEqual(a, b)

    @unittest.skipUnless(have_numpy, "requires build with numpy-support")
    def test_numpy_conversion_0(self):
        a = histogram(integer(0, 3, uoflow=False))
        for i in range(10):
            a.fill(1)
        a.fill(1, count=90)
        c = numpy.array(a) # a copy
        v = numpy.asarray(a) # a view

        for t in (c, v):
            self.assertEqual(t.dtype, numpy.uint8)
            self.assertTrue(numpy.all(t == numpy.array((0, 100, 0))))

        for i in range(20):
            a.fill(1)
        a.fill(1, count=2 * numpy.ones(40, dtype=numpy.uint32))
        # copy does not change, but view does
        self.assertTrue(numpy.all(c == numpy.array((0, 100, 0))))
        self.assertTrue(numpy.all(v == numpy.array((0, 200, 0))))

        a.fill(1, count=100)
        c = numpy.array(a)
        self.assertEqual(c.dtype, numpy.uint16)
        self.assertTrue(numpy.all(c == numpy.array((0, 300, 0))))
        # view does not follow underlying switch in word size
        self.assertFalse(numpy.all(c == v))

    @unittest.skipUnless(have_numpy, "requires build with numpy-support")
    def test_numpy_conversion_1(self):
        a = histogram(integer(0, 3))
        for i in range(10):
            a.fill(1, weight=3)
        c = numpy.array(a) # a copy
        v = numpy.asarray(a) # a view
        self.assertEqual(c.dtype, numpy.float64)
        self.assertTrue(numpy.all(c == numpy.array(((0, 30, 0, 0, 0), (0, 90, 0, 0, 0)))))
        self.assertTrue(numpy.all(v == c))

    @unittest.skipUnless(have_numpy, "requires build with numpy-support")
    def test_numpy_conversion_2(self):
        a = histogram(integer(0, 2, uoflow=False),
                      integer(0, 3, uoflow=False),
                      integer(0, 4, uoflow=False))
        r = numpy.zeros((2, 3, 4), dtype=numpy.int8)
        for i in range(len(a.axis(0))):
            for j in range(len(a.axis(1))):
                for k in range(len(a.axis(2))):
                    for m in range(i+j+k):
                        a.fill(i, j, k)
                    r[i,j,k] = i+j+k

        d = numpy.zeros((2, 3, 4), dtype=numpy.int8)
        for i in range(len(a.axis(0))):
            for j in range(len(a.axis(1))):
                for k in range(len(a.axis(2))):
                    d[i,j,k] = a.value(i,j,k)

        self.assertTrue(numpy.all(d == r))

        c = numpy.array(a) # a copy
        v = numpy.asarray(a) # a view

        self.assertTrue(numpy.all(c == r))
        self.assertTrue(numpy.all(v == r))

    @unittest.skipUnless(have_numpy, "requires build with numpy-support")
    def test_numpy_conversion_3(self):
        a = histogram(integer(0, 2),
                      integer(0, 3),
                      integer(0, 4))
        r = numpy.zeros((2, 4, 5, 6))
        for i in range(len(a.axis(0))):
            for j in range(len(a.axis(1))):
                for k in range(len(a.axis(2))):
                    a.fill(i, j, k, weight=i+j+k)
                    r[0, i, j, k] = i+j+k
                    r[1, i, j, k] = (i+j+k)**2
        c = numpy.array(a) # a copy
        v = numpy.asarray(a) # a view

        c2 = numpy.zeros((2, 4, 5, 6))
        for i in range(len(a.axis(0))):
            for j in range(len(a.axis(1))):
                for k in range(len(a.axis(2))):
                    c2[0, i, j, k] = a.value(i,j,k)
                    c2[1, i, j, k] = a.variance(i,j,k)

        self.assertTrue(numpy.all(c == c2))
        self.assertTrue(numpy.all(c == r))
        self.assertTrue(numpy.all(v == r))

    @unittest.skipUnless(have_numpy, "requires build with numpy-support")
    def test_numpy_conversion_4(self):
        a = histogram(integer(0, 2, uoflow=False),
                      integer(0, 4, uoflow=False))
        a1 = numpy.asarray(a)
        self.assertEqual(a1.dtype, numpy.uint8)
        self.assertEqual(a1.shape, (2, 4))

        b = histogram()
        b1 = numpy.asarray(b)
        self.assertEqual(b1.shape, (0,))
        self.assertEqual(numpy.sum(b1), 0)

    @unittest.skipUnless(have_numpy, "requires build with numpy-support")
    def test_numpy_conversion_5(self):
        a = histogram(integer(0, 3, uoflow=False),
                      integer(0, 2, uoflow=False))
        a.fill(0, 0)
        for i in range(80):
            a += a
        # a now holds a multiprecision type
        a.fill(1, 0)
        for i in range(2): a.fill(2, 0)
        for i in range(3): a.fill(0, 1)
        for i in range(4): a.fill(1, 1)
        for i in range(5): a.fill(2, 1)
        a1 = numpy.asarray(a)
        self.assertEqual(a1.shape, (3, 2))
        self.assertEqual(a1[0, 0], float(2 ** 80))
        self.assertEqual(a1[1, 0], 1)
        self.assertEqual(a1[2, 0], 2)
        self.assertEqual(a1[0, 1], 3)
        self.assertEqual(a1[1, 1], 4)
        self.assertEqual(a1[2, 1], 5)

    @unittest.skipUnless(have_numpy, "requires build with numpy-support")
    def test_numpy_conversion_6(self):
        a = integer(0, 2)
        b = regular(2, 0, 2)
        c = variable(0, 1, 2)
        ref = numpy.array((0., 1., 2.))
        self.assertTrue(numpy.all(numpy.array(a) == ref))
        self.assertTrue(numpy.all(numpy.array(b) == ref))
        self.assertTrue(numpy.all(numpy.array(c) == ref))
        d = circular(4)
        ref = numpy.array((0., 0.5*pi, pi, 1.5*pi, 2.0*pi))
        self.assertTrue(numpy.all(numpy.array(d) == ref))
        e = category(1, 2)
        ref = numpy.array((1, 2))
        self.assertTrue(numpy.all(numpy.array(e) == ref))

    @unittest.skipUnless(have_numpy, "requires build with numpy-support")
    def test_fill_with_numpy_array_0(self):
        ar = lambda *args: numpy.array(args, dtype=float)
        a = histogram(integer(0, 3, uoflow=False))
        a.fill(ar(-1, 0, 1, 2, 1))
        a.fill((4, -1, 0, 1, 2))
        self.assertEqual(a.value(0), 2)
        self.assertEqual(a.value(1), 3)
        self.assertEqual(a.value(2), 2)

        with self.assertRaises(ValueError):
            a.fill(numpy.empty((2, 2)))
        with self.assertRaises(ValueError):
            a.fill(numpy.empty(2), 1)
        with self.assertRaises(ValueError):
            a.fill("abc")

        a = histogram(integer(0, 2, uoflow=False),
                      regular(2, 0, 2, uoflow=False))
        a.fill(ar(-1, 0, 1),
               ar(-1., 1., 0.1))
        self.assertEqual(a.value(0, 0), 0)
        self.assertEqual(a.value(0, 1), 1)
        self.assertEqual(a.value(1, 0), 1)
        self.assertEqual(a.value(1, 1), 0)

        with self.assertRaises(ValueError):
            a.fill(ar(1, 2, 3))

        a = histogram(integer(0, 3, uoflow=False))
        a.fill(ar(0, 0, 1, 2, 1, 0, 2, 2))
        self.assertEqual(a.value(0), 3)
        self.assertEqual(a.value(1), 2)
        self.assertEqual(a.value(2), 3)


    @unittest.skipUnless(have_numpy, "requires build with numpy-support")
    def test_fill_with_numpy_array_1(self):
        ar = lambda *args: numpy.array(args, dtype=float)
        a = histogram(integer(0, 3, uoflow=True))
        v = ar(-1, 0, 1, 2, 3, 4)
        w = ar( 2, 3, 4, 5, 6, 7)
        a.fill(v, weight=w)
        a.fill((0, 1), weight=(2, 3))
        self.assertEqual(a.value(-1), 2)
        self.assertEqual(a.value(0), 5)
        self.assertEqual(a.value(1), 7)
        self.assertEqual(a.value(2), 5)
        self.assertEqual(a.variance(-1), 4)
        self.assertEqual(a.variance(0), 13)
        self.assertEqual(a.variance(1), 25)
        self.assertEqual(a.variance(2), 25)
        a.fill((1, 2), weight=1)
        a.fill(0, weight=(1, 2))
        self.assertEqual(a.value(0), 8)
        self.assertEqual(a.value(1), 8)
        self.assertEqual(a.value(2), 6)

        with self.assertRaises(RuntimeError):
            a.fill((1, 2), foo=(1, 1))
        with self.assertRaises(ValueError):
            a.fill((1, 2), weight=(1,))
        with self.assertRaises(ValueError):
            a.fill((1, 2), weight="ab")
        with self.assertRaises(RuntimeError):
            a.fill((1, 2), weight=(1, 1), foo=1)
        with self.assertRaises(ValueError):
            a.fill((1, 2), weight=([1, 1], [2, 2]))

        a = histogram(integer(0, 2, uoflow=False),
                      regular(2, 0, 2, uoflow=False))
        a.fill((-1, 0, 1), (-1, 1, 0.1))
        self.assertEqual(a.value(0, 0), 0)
        self.assertEqual(a.value(0, 1), 1)
        self.assertEqual(a.value(1, 0), 1)
        self.assertEqual(a.value(1, 1), 0)
        a = histogram(integer(0, 3, uoflow=False))
        a.fill((0, 0, 1, 2))
        a.fill((1, 0, 2, 2))
        self.assertEqual(a.value(0), 3)
        self.assertEqual(a.value(1), 2)
        self.assertEqual(a.value(2), 3)

if __name__ == "__main__":
    unittest.main()
