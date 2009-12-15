import Config
from SCons.Script import *

dbgFlags = {}
optdbgFlags = {}
releaseFlags = {}

env = Environment()
if env['CC'] == 'gcc':
   dbgFlags['CCFLAGS'] = ['-g', '-O0']
   optdbgFlags['CCFLAGS'] = ['-g', '-O2']
   releaseFlags['CCFLAGS'] = ['-O2']
elif env['CC'] == 'cl':
   dbgFlags['CCFLAGS'] = ['/Od']
   dbgFlags['PDB'] = '${TARGET}.pdb'
   optdbgFlags['CCFLAGS'] = ['/O2']
   optdbgFlags['PDB'] = '${TARGET}.pdb'
   releaseFlags['CCFLAGS'] = ['/O2']

dbg = Config.Config('dbg', suffix='_DEBUG', **dbgFlags)
optdbg = Config.Config('optdbg', suffix='_OPTIMIZED', **optdbgFlags)
release = Config.Config('release', default=True, **releaseFlags)
