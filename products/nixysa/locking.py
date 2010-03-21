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

"""This file implements flock-style file locking for use on Mac,
Windows, or Linux.
"""

import sys

if sys.platform == "win32":
  import pywintypes
  import win32con
  import win32file

  # Export the "standard" names for flock-style locking.
  LOCK_SH = 0
  LOCK_NB = win32con.LOCKFILE_FAIL_IMMEDIATELY # 1
  LOCK_EX = win32con.LOCKFILE_EXCLUSIVE_LOCK  # 2
  LOCK_UN = 4

  overlapped = pywintypes.OVERLAPPED()
  overlapped.hEvent = 0
  overlapped.Offset = 0

  def lockf(fd, operation):
    file_handle = win32file._get_osfhandle(fd.fileno())
    if operation == LOCK_UN:
      try:
        win32file.UnlockFileEx(file_handle, 0, -0x7fff0000, overlapped)
      except pywintypes.error, e:
        if e[0] == 158:
          # Ignore error that happens when you unlock an already
          # unlocked file.
          pass
        else:
          raise
    else:
      try:
        win32file.LockFileEx(file_handle, operation, 0, -0x7fff0000, overlapped)
      except pywintypes.error, e:
        if e[0] == 33:
          raise Exception("Lock on fileno %d failed." % file.fileno())
        else:
          raise
else:
  import fcntl

  # Export the "standard" names for flock-style locking.
  LOCK_EX = fcntl.LOCK_EX
  LOCK_NB = fcntl.LOCK_NB
  LOCK_SH = fcntl.LOCK_SH
  LOCK_UN = fcntl.LOCK_UN

  def lockf(fd, operation):
    fcntl.lockf(fd, operation)
