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

"""Logging functions.

This module has functions for logging errors and warnings.
"""

import sys


_num_errors = 0
_num_warnings = 0


def Error(msg):
  """Prints an error."""
  global _num_errors
  _num_errors += 1
  print >> sys.stderr, ('ERROR: %s' % msg)


def Warning(msg):
  """Prints an warning."""
  global _num_warnings
  _num_warnings += 1
  print >> sys.stderr, ('WARNING: %s' % msg)


def Info(msg):
  """Prints Info."""
  print msg


def SourceError(source, msg):
  """Prints an error with source info"""
  Error('%s:%d %s' % (source.file.source, source.line, msg))


def SourceWarning(source, msg):
  """Prints an warning with source info"""
  Warning ('%s:%d %s' % (source.file.source, source.line, msg))


def FailIfHaveErrors():
  """Print status and exit if there were errors."""
  global _num_errors
  global _num_warnings
  if _num_errors > 0 or _num_warnings > 0:
    print >> sys.stderr, 'Num Errors:', _num_errors
    print >> sys.stderr, 'Num Warnings:', _num_warnings
  if _num_errors > 0:
    sys.exit(1)
