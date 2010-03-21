from SCons.Script import *

def TOOL_REZ(env):
   bld = Builder(action = 'rez -o $TARGET -useDF -script Roman $SOURCE',
                 suffix = '.rsrc', src_suffix='.r')
   env['BUILDERS']['REZ'] = bld
