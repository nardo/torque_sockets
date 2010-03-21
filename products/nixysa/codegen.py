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

"""Code generator

This file is the main entry point for the code generator.
To use:
 codegen.py --output-dir=output-path --generate=npapi file1.idl file2.idl ...
"""

import glob
import imp
import md5
import os
import sys

import gflags

# local imports
import idl_parser
import locking
import log
import syntax_tree

# default supported generators
import header_generator
import cpp_header_generator
import js_header_generator
import npapi_generator

# default supported binding models
import pod_binding
import enum_binding
import callback_binding
import by_value_binding
import by_pointer_binding
import unsized_array_binding
import nullable_binding

generators = {'header': header_generator,
              'cppheader': cpp_header_generator,
              'jsheader': js_header_generator,
              'npapi': npapi_generator}

binding_models = {'pod': pod_binding,
                  'callback': callback_binding,
                  'enum': enum_binding,
                  'by_value': by_value_binding,
                  'by_pointer': by_pointer_binding,
                  'unsized_array': unsized_array_binding,
                  'nullable': nullable_binding}


FLAGS = gflags.FLAGS
gflags.DEFINE_multistring('binding-module', [], 'include a binding model'
                          ' module. Value is name:path where \'name\' is the'
                          ' binding model name, and \'path\' is the binding'
                          ' model module path.')

gflags.DEFINE_multistring('generator-module', [], 'include a generator module.'
                          ' Value is name:path where \'name\' is the generator'
                          ' name, and \'path\' is the generator module path.')

gflags.DEFINE_multistring('generate', [], 'the generator to use')
gflags.DEFINE_string('output-dir', '.', 'the output directory')

gflags.DEFINE_boolean('exclusive-lock', False, 'Use file locking to make sure'
                      ' there is only one instance running at a time.')
gflags.DEFINE_boolean('force', False, 'force generation even if the source'
                      ' files have not changed')
gflags.DEFINE_boolean('force-docs', False, 'force all members to have'
                      ' documentation blocks or else raise an exception.')
gflags.DEFINE_boolean('no-return-docs', False, 'remove docs marked as'
                      ' noreturndocs.')
gflags.DEFINE_boolean('properties-equal-undefined', False,
                      'Emit class.prototype.property = undefined;')

class NativeType(syntax_tree.Definition):
  defn_type = 'Native'

  def __init__(self, source, attributes, name, podtype):
    syntax_tree.Definition.__init__(self, source, attributes, name)
    self.podtype = podtype
    self.is_type = True

  def LookUpBindingModel(self):
    """Implementation of LookUpBindingModel for NativeType."""
    return 'pod'


def GetStdNamespace():
  pod_attributes = {'binding_model': 'pod'}
  source_file = idl_parser.File('<internal>')
  source_file.header = "common.h"
  source_file.npapi_cpp = None
  source_file.npapi_header = None
  source = idl_parser.SourceLocation(source_file, 0)
  defn_list = [NativeType(source, pod_attributes, 'string', 'string'),
               NativeType(source, pod_attributes, 'wstring', 'wstring')]
  return syntax_tree.Namespace(source, [], 'std', defn_list)


def GetNativeTypes():
  pod_attributes = {'binding_model': 'pod'}
  source_file = idl_parser.File('<internal>')
  source_file.header = None
  source_file.npapi_cpp = None
  source_file.npapi_header = None
  source = idl_parser.SourceLocation(source_file, 0)
  return [NativeType(source, pod_attributes, 'void', 'void'),
          NativeType(source, pod_attributes, 'int', 'int'),
          NativeType(source, pod_attributes, 'unsigned int', 'int'),
          NativeType(source, pod_attributes, 'size_t', 'int'),
          NativeType(source, pod_attributes, 'bool', 'bool'),
          NativeType(source, pod_attributes, 'float', 'float'),
          NativeType(source, pod_attributes, 'double', 'float'),
          NativeType(source, pod_attributes, 'Variant', 'variant'),
          GetStdNamespace()]


def AddModulesFromFlags(table, flag_values, md5_hash):
  for entry in flag_values:
    string_list = entry.split(':')
    name = string_list[0]
    path = ':'.join(string_list[1:])
    try:
      # hash the extra modules that we load
      md5_hash.update(open(path).read())
      table[name] = imp.load_source(name, path)
    except IOError:
      print 'Could not load module %s.' % path
      raise


def main(argv):
  files = argv[1:]
  # generate a hash of all the inputs to figure out if we need to re-generate
  # the outputs.
  md5_hash = md5.new()
  # hash the input files and the source python files (globbing *.py in the
  # directory of this file)
  for source_file in files + glob.glob(os.path.join(os.path.dirname(__file__),
                                                    '*.py')):
    md5_hash.update(open(source_file).read())
  # hash the options since they may affect the output
  for s in (FLAGS['generator-module'].value + FLAGS['binding-module'].value +
            FLAGS.generate + [FLAGS['output-dir'].value]):
    md5_hash.update(s)

  # import generator and binding model modules, and hash them
  AddModulesFromFlags(generators, FLAGS['generator-module'].value, md5_hash)
  AddModulesFromFlags(binding_models, FLAGS['binding-module'].value, md5_hash)

  output_dir = FLAGS['output-dir'].value
  if not os.path.isdir(output_dir):
    os.makedirs(output_dir)

  hash_filename = os.path.join(output_dir, 'hash')
  hash_value = md5_hash.hexdigest()
  if not FLAGS.force:
    try:
      hash_file = open(hash_filename, 'r')
      # Don't read while others are writing...
      if FLAGS['exclusive-lock'].value:
        locking.lockf(hash_file, locking.LOCK_SH)

      old_hash = hash_file.read()

      if FLAGS['exclusive-lock'].value:
        locking.lockf(hash_file, locking.LOCK_UN)
      hash_file.close()

      if hash_value == old_hash:
        print "Source files haven't changed: nothing to generate."
        return
    except IOError:
      # Could not load the hash file, so there must be stuff to
      # generate.
      pass

  hash_file = open(hash_filename, 'w')
  if FLAGS['exclusive-lock'].value:
    locking.lockf(hash_file, locking.LOCK_EX)

  my_parser = idl_parser.Parser(output_dir)
  pairs = []
  for f in files:
    idl_file = idl_parser.File(f)
    defn = my_parser.Parse(idl_file)
    pairs.append((idl_file, defn))
  definitions = sum([defn for (f, defn) in pairs], []) + GetNativeTypes()
  global_namespace = syntax_tree.Namespace(None, [], '', definitions)
  syntax_tree.FinalizeObjects(global_namespace, binding_models)

  writer_list = []
  for generator_name in FLAGS.generate:
    try:
      generator = generators[generator_name]
      writer_list += generator.ProcessFiles(output_dir, pairs, global_namespace)
    except KeyError:
      print 'Unknown generator %s.' % generator_name
      raise
  for writer in writer_list:
    writer.Write()

  # Save hash for next time
  hash_file.write(hash_value)
  if FLAGS['exclusive-lock'].value:
    locking.lockf(hash_file, locking.LOCK_UN)
  hash_file.close()
  log.FailIfHaveErrors()


if __name__ == '__main__':
  main(FLAGS(sys.argv))
