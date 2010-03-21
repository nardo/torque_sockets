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

"""Identifier renaming module.

This module implements a few functions to rename identifiers according to some
naming conventions.

Normalize is the main function, allowing to convert an identifier using an
arbitrary naming convention into another naming convention. For example:

naming.Normalize("MyIdentifier", naming.Upper) will return "MY_IDENTIFIER".

When the same identifier has to be converted into several conventions, it is
more efficient to use SplitWords once, and call the conversion functions
directly:

words = naming.SplitWords("MyIdentifier")
print naming.Upper(words)
-> MY_IDENTIFIER
print naming.Lower(words)
-> my_identifier
print naming.LowerTrailing(words)
-> my_identifier_
print naming.Java(words)
-> myIdentifier
print naming.Capitalized(words)
-> MyIdentifier
"""

import re


def Upper(words):
  """Makes an upper-case identifier from words.

  Args:
    words: a list of lower-case words.

  Returns:
    the upper-case identifier.
  """
  return '_'.join(s.upper() for s in words)


def Lower(words):
  """Makes a lower-case identifier from words.

  Args:
    words: a list of lower-case words.

  Returns:
    the lower-case identifier.
  """
  return '_'.join(words)


def LowerTrailing(words):
  """Makes a lower-case with trailing underscore identifier from words.

  Args:
    words: a list of lower-case words.

  Returns:
    the lower-case with trailing underscore identifier.
  """
  return Lower(words) + '_'


def Java(words):
  """Makes a java-like identifier from words.

  Args:
    words: a list of lower-case words.

  Returns:
    the java-like identifier.
  """
  return words[0] + Capitalized(words[1:])


def Capitalized(words):
  """Makes a capitalized identifier from words.

  Args:
    words: a list of lower-case words.

  Returns:
    the capitalized identifier.
  """
  return ''.join(s.capitalize() for s in words)


def SplitWords(input_string):
  """Transforms a input_string into a list of lower-case components.

  Args:
    input_string: the input string.

  Returns:
    a list of lower-case words.
  """
  if input_string.find('_') > -1:
    # 'some_TEXT_' -> 'some text'
    return input_string.replace('_', ' ').strip().lower().split()
  else:
    if re.search('[A-Z]', input_string) and re.search('[a-z]', input_string):
      # mixed case.
      # look for capitalization to cut input_strings
      # 'SomeText' -> 'Some Text'
      input_string = re.sub('([A-Z])', r' \1', input_string).strip()
      # 'Vector3' -> 'Vector 3'
      input_string = re.sub('([^0-9])([0-9])', r'\1 \2', input_string)
    return input_string.lower().split()


def Normalize(input_string, func):
  """Normalizes an identifier into a particular case.

  Args:
    input_string: the input string.
    func: a function that takes a list of lower-case words and returns a
      string. Upper, Lower, LowerTrailing, Java and Capitalized are good
      candidates.

  Returns:
    the normalized identifier.
  """
  return func(SplitWords(input_string))


def main():
  pass


if __name__ == '__main__':
  main()
