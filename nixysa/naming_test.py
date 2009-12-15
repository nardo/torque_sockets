#!/usr/bin/python2.4
#
# Copyright 2008 Google Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""Test for naming."""

import unittest
import naming


class NamingUnitTest(unittest.TestCase):
  _test_vectors = [(['foo'],
                    {naming.Upper: 'FOO',
                     naming.Lower: 'foo',
                     naming.LowerTrailing: 'foo_',
                     naming.Java: 'foo',
                     naming.Capitalized: 'Foo'}),
                   (['foo', 'bar'],
                    {naming.Upper: 'FOO_BAR',
                     naming.Lower: 'foo_bar',
                     naming.LowerTrailing: 'foo_bar_',
                     naming.Java: 'fooBar',
                     naming.Capitalized: 'FooBar'}),
                   (['foo', 'bar', 'baz'],
                    {naming.Upper: 'FOO_BAR_BAZ',
                     naming.Lower: 'foo_bar_baz',
                     naming.LowerTrailing: 'foo_bar_baz_',
                     naming.Java: 'fooBarBaz',
                     naming.Capitalized: 'FooBarBaz'}),
                   (['rotation', 'x', 'y', 'z'],
                    {naming.Upper: 'ROTATION_X_Y_Z',
                     naming.Lower: 'rotation_x_y_z',
                     naming.LowerTrailing: 'rotation_x_y_z_',
                     naming.Java: 'rotationXYZ',
                     naming.Capitalized: 'RotationXYZ'}),
                   (['vector', '3', 'array'],
                    {naming.Upper: 'VECTOR_3_ARRAY',
                     naming.Lower: 'vector_3_array',
                     naming.LowerTrailing: 'vector_3_array_',
                     naming.Java: 'vector3Array',
                     naming.Capitalized: 'Vector3Array'})]

  def testFunctions(self):
    for words, results in self._test_vectors:
      for func in results:
        self.assertEquals(func(words), results[func])

  def testSplitWords(self):
    for words, results in self._test_vectors:
      for func in results:
        self.assertEquals(naming.SplitWords(results[func]), words)

  def testNormalize(self):
    for unused_words, results in self._test_vectors:
      for func_source in results:
        source = results[func_source]
        for func_dest in results:
          dest = results[func_dest]
          self.assertEquals(naming.Normalize(source, func_dest), dest)


if __name__ == '__main__':
  unittest.main()
