from SCons.Script import *
from SCons.Script.SConscript import SConsEnvironment
from SCons.Node import *
from string import Template

class SubstDeps:
   def __call__(self, env, target, **kw):
      env.Depends(target, SCons.Node.Python.Value(str(kw['SUBST_DICT'])))

def subst_file(target, source, env):
   # Read file
   f = open(source, 'r')
   try:
      contents = f.read()
   finally:
      f.close()

   # Substitute
   t = Template(contents)
   contents = t.safe_substitute(env['SUBST_DICT'])

   # Write file
   f = open(target, 'w')
   try:
      f.write(contents)
   finally:
      f.close()

def subst_in_file(target, source, env):
   # Substitute in the files
   for (t, s) in zip(target, source):
      subst_file(str(t), str(s), env)

   return 0

def subst_string(target, source, env):
   items = ['Substituting vars from %s to %s' % (str(s), str(t))
            for (t, s) in zip(target, source)]

   return '\n'.join(items)

# Create builders
def TOOL_SUBST(env):
   subst_in_file_action = SCons.Action.Action(subst_in_file, subst_string)
   env['BUILDERS']['SubstInFile'] = Builder(action=subst_in_file_action)
   if not 'CUSTOM_DEPS' in env:
      env['CUSTOM_DEPS'] = {}
   env['CUSTOM_DEPS']['SubstInFile'] = SubstDeps()

def TOOL_BUNDLE(env):
   """defines env.MakeBundle() for installing a bundle into its dir.
      A bundle has this structure: (filenames are case SENSITIVE)
      sapphire.bundle/
       Contents/
          Info.plist (an XML key->value database)
          MacOS/
             executable (the executable or shared lib)
      Resources/
       """
   if 'BUNDLE' in env['TOOLS']: return
   if env['PLATFORM'] == 'darwin':
      TOOL_SUBST(env)
      # Common type codes are BNDL for generic bundle and APPL for application.
      def MakeBundle(env, bundledir, app,
                    info_plist,
                    typecode='BNDL', creator='????',
                    resources=[]):
         """Install a bundle into its dir, in the proper format"""
         # Substitute construction vars:
         for a in [bundledir, info_plist, typecode, creator]:
            a = env.subst(a)

         if SCons.Util.is_List(app):
            app = app[0]
         if SCons.Util.is_String(app):
            app = env.subst(app)
            appbase = app
         else:
            appbase = str(app)

         if not ('.' in bundledir):
            bundledir += '.$BUNDLEDIRSUFFIX'

         bundledir = env.subst(bundledir) # substitute again
         Mkdir(bundledir)
         env.Clean(app, bundledir)

         env.Install(bundledir+'/Contents/MacOS', app)
         env.SubstInFile(bundledir+'/Contents/Info.plist', info_plist,
                         SUBST_DICT={'EXECUTABLE_NAME':app,
                         'PRODUCT_NAME':app})

         if SCons.Util.is_String(resources):
            resources = [resources]
         for r in resources:
            t = env.Install(bundledir+'/Contents/Resources', env.subst(r))
            env.Clean(t, t)
         return [ SCons.Node.FS.default_fs.Dir(bundledir) ]
      # This is not a regular Builder; it's a wrapper function.
      # So just make it available as a method of Environment.
      SConsEnvironment.MakeBundle = MakeBundle
