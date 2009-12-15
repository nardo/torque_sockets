#!/usr/bin/python2.4
#
# Copyright 2009 Google Inc.
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

"""File writing functions.

This module contain function to write files only if their contents would
change.
"""

import os.path
import log


def WriteIfContentDifferent(filename, content):
  """Write file only if content is different or if filename does not exist.

  Args:
    filename: filename of file.
    content: string containing contents of file.
  """
  if os.path.exists(filename):
    f = open(filename, 'r');
    old_content = f.read()
    f.close()
    if old_content == content:
      return
  f = open(filename, 'w')
  f.write(content)
  f.close()
  log.Info('Writing %s' % filename)

