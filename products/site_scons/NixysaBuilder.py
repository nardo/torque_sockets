from SCons.Script import *
from sys import *

def TOOL_NIXYSA(env):
   if sys.platform == 'win32':
      env.Append(CODEGEN = 'codegen.bat')
   elif sys.platform == 'darwin':
      env.Append(CODEGEN = 'codegen.sh')
   else:
      env.Append(CODEGEN = 'codegen.sh')

   env.Append(ROOT = '../..',
              NIXYSA_DIR = '$ROOT/nixysa',
              STATIC_GLUE_DIR = '$NIXYSA_DIR/static_glue/npapi',
              NPAPI_DIR = '$ROOT/third_party/npapi/include',
              GLUE_DIR = env.Dir('glue'),
              CPPPATH=['$STATIC_GLUE_DIR', '$NPAPI_DIR', '$GLUE_DIR'])

   env.Append(ENV={'PYTHON': sys.executable})
   def NixysaEmitter(target, source, env):
      bases = [os.path.splitext(s.name)[0] for s in source] + ['globals']
      targets = ['$GLUE_DIR/%s_glue.cc' % b for b in bases]
      targets += ['$GLUE_DIR/%s_glue.h' % b for b in bases]
      targets += ['$GLUE_DIR/hash', '$GLUE_DIR/parsetab.py']
      return targets, source

   NIXYSA_CMDLINE = ' '.join(['"' + env.File('$NIXYSA_DIR/$CODEGEN').path + '"',
                              '--output-dir="' + env.Dir('$GLUE_DIR').path + '"',
                              '--generate=npapi',
                              '$SOURCES'])

   env['BUILDERS']['Nixysa'] = Builder(action=NIXYSA_CMDLINE,
                                       emitter=NixysaEmitter,
                                       src_suffix='.idl',
                                       suffix='.cc')
