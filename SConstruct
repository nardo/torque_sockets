# Begin code that does stuff
import Config
import DefaultConfig
import Universal

vars = Variables()
Config.GetVars(vars)
Universal.GetVars(vars)
env = Environment(variables=vars)

Export('env')

libs = env.SConscript('platform_library/SConscript')
env.SConscript('test/test2/SConscript', 'libs')
env.SConscript('plugin/tnp/SConscript', 'libs')
