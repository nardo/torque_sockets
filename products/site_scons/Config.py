from SCons.Script import *
from SCons.Tool import *
import Universal
from os import path

class Config(object):
   allConfigs = {}
   default = 'none'
   def __init__(self, name, default=False, suffix='', **kw):
      self.name = name
      self.opts = kw
      self.suffix = suffix
      Config.allConfigs[self.name] = self
      if default:
         Config.default = self.name

   def modenv(self, env):
      newEnv = env.Clone()
      newEnv.Prepend(**self.opts)
      return newEnv

def _addResBuilder(env):
   """This adds the Windows Resource Script builder 'RES' to every builder which
   generates an executable or library output.  This is necessary to get proper
   output when a .rc file is specified as input, otherwise SCons tries to
   directly link the .rc file instead of compiling it first."""
   if env['PLATFORM'] == 'win32':
      ld = SCons.Tool.createLoadableModuleBuilder(environ)
      ld.add_src_builder('RES')

      sl = SCons.Tool.createSharedLibBuilder(environ)
      sl.add_src_builder('RES')

      sl = SCons.Tool.createStaticLibBuilder(environ)
      sl.add_src_builder('RES')

      p = SCons.Tool.createProgBuilder(environ)
      p.add_src_builder('RES')

def _getConfigurationEnvironment(env, config, **kw):
   newEnv = config.modenv(env)
   cleanDict = {}
   for key in kw.keys():
      if key.endswith('_CFG'):
         cleanDict[key[:-4]] = kw[key][config.name]
      else:
         cleanDict[key] = kw[key]

   return (newEnv, cleanDict)

def _getConfigFile(file, cfg):
   if not SCons.Util.is_Dict(file):
      return file

   return file[cfg] if cfg in file else None

def _handleCustomDependencies(env, command, file, *args, **kw):
   deps = []
   if 'CUSTOM_DEPS' in env:
      deps = env['CUSTOM_DEPS']
   if command in deps:
      deps[command](env, file, *args, **kw)

def Build(environ, command, target, source, copy=True, suffix=True, *args, **kw):
   _addResBuilder(environ)
   configs = environ.subst('${cfg}').split()
   fullPath = {}
   cleanName = {}

   # Build each config
   for config in configs:
      config = Config.allConfigs[config]

      # Insert the configuration options
      newEnv, cleanDict = _getConfigurationEnvironment(environ, config, **kw)
      newEnv.Append(**cleanDict)

      # Make sure source is a list
      if not SCons.Util.is_List(source):
         source = [source]

      # Tell SCons to build the files in a build/<cfg> directory
      outdir = 'build/' + config.name + '/'
      newFiles = []
      for file in source:
         file = _getConfigFile(file, config.name)
         if not file:
            continue

         fileNode = newEnv.File(file)
         file = str(file)
         dir, fileName = path.split(newEnv.subst(file))

         # We may get some files which are already in the output directory but
         # need further processing.  In those cases, don't put them in
         # build/<cfg>/build/<cfg> :)
         if path.normpath(dir) == path.normpath(outdir):
            dir = ''
         outdirdir = outdir + dir.strip('./')
         newFile = outdirdir + '/' + fileName

         # Here there be dragons, of sorts.
         #
         # In the event that the input file *is* being built, but *is not* being
         # built into our VariantDir, SCons gets horribly confused.  See,
         # has_builder() (and therefore is_derived()) is *also* used as the flag
         # for "I should be in the VariantDir even if duplicate is 0, so don't
         # go looking anywhere else for me".  So if we create a new
         # build/<cfg>/file here out of thin air, SCons *won't* go to the source
         # file because it thinks it already should be in build/<cfg>/file
         #
         # At one point I had done some remarkably stupid things here, which
         # worked, if only just (basically saying that there is a file at
         # build/<cfg>/file which has the same sources and builders as file, but
         # is a symlink to file).  The much simpler solution is to simply copy
         # file to build/<cfg>/file ourselves.  This will create a file at
         # build/<cfg>/file and mark it as depending on file.
         if path.normpath(newFile) != path.normpath(file) and fileNode.is_derived():
            newEnv.Command(newFile, fileNode, Copy('$TARGET', '$SOURCE'))

         newFiles.append(newFile)

         # SCons doesn't like pointing a VariantDir at the empty string.
         if not dir:
            dir = '.'

         # New directory, new Variant Dir
         newEnv.VariantDir(outdirdir, dir, duplicate=0)

      # Add the suffix if applicable
      newTarget = target + config.suffix if suffix else target
      # Actually build the thing.  Returns a list of built targets, we assume
      # that there is only one element (of importance at least) in the list.
      p = newEnv.DoUniversal(command, outdir + newTarget, newFiles, *args)

      # Copy the final product into our current working directory and save
      # that path for dependency tracking.
      if copy:
         fullPath[config.name] = newEnv.Command(path.split(str(p[0]))[1], p,
                                                Copy('$TARGET', '$SOURCE'))
      else:
         fullPath[config.name] = p[0]

      # Save the unadorned name of the product to pass into other build comamnds
      cleanName[config.name] = newTarget

      # Handle any custom dependency tracking that may be necessary once we have
      # the full adorned product name.
      _handleCustomDependencies(newEnv, command, fullPath[config.name], *args, **cleanDict)

   return fullPath, cleanName

def GetVars(vars):
   vars.Add(ListVariable('cfg', 'Build configurations', Config.default,
                         Config.allConfigs.keys()))

AddMethod(Environment, Build)
