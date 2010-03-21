from SCons.Script import *
from os import path

def GetVars(vars):
   vars.Add(ListVariable('arch', 'Build architectures', 'all',
                         ['ppc','i386','x86_64']))

def DoUniversal(env, command, target, source, *args, **kw):
   envs = []
   builder = env['BUILDERS'][command]
   exe = (command == 'LoadableModule' or command == 'SharedLibrary' or command == 'StaticLibrary' or command == 'Program' or command == 'Library')
   if env['PLATFORM'] == 'darwin' and exe:
      archs = env.subst('${arch}').split()
      outs = []
      for arch in archs:
         newEnv = env.Clone()
         newEnv.Append(CCFLAGS="-arch " + arch, LINKFLAGS="-arch " + arch, OBJPREFIX=arch)
         outs += builder(newEnv, target=None, source=source, *args, **kw)

      p, f = path.split(target)
      target = path.join(p, builder.get_prefix(env) + f + builder.get_suffix(env))
      ret = env.Command(target, outs, "lipo -create $SOURCES -output $TARGET" )
   else:
      ret = builder(env, target, source, *args, **kw)

   return ret;

AddMethod(Environment, DoUniversal)
