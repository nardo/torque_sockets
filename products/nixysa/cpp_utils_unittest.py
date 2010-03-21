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

"""Test for cpp_utils."""

import unittest
import cpp_utils

template = """test1
${#Test}
test2"""

template_reuse = """test1
${#Test}
test2
${#Test}
test3"""


class CppFileWriterUnitTest(unittest.TestCase):
  def setUp(self):
    self.writer = cpp_utils.CppFileWriter('a.cc', False)

  def tearDown(self):
    pass

  def testSectionTemplate(self):
    section = self.writer.CreateSection('test')
    section.EmitTemplate(template)
    self.assertNotEquals(section.GetSection('Test'), None)
    test_section = section.GetSection('Test')
    test_section.EmitCode('test3')
    lines = section.GetLines()
    self.assertTrue(lines[0] == 'test1')
    self.assertTrue(lines[1] == 'test3')
    self.assertTrue(lines[2] == 'test2')

  def testSectionTemplateReuse(self):
    section = self.writer.CreateSection('test')
    section.EmitTemplate(template_reuse)
    self.assertNotEquals(section.GetSection('Test'), None)
    test_section = section.GetSection('Test')
    test_section.EmitCode('test4')
    lines = section.GetLines()
    self.assertTrue(lines[0] == 'test1')
    self.assertTrue(lines[1] == 'test4')
    self.assertTrue(lines[2] == 'test2')
    self.assertTrue(lines[3] == 'test4')
    self.assertTrue(lines[4] == 'test3')


if __name__ == '__main__':
  unittest.main()
