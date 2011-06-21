#
# Copyright (C) 2011 Alex Smith
#
# Permission to use, copy, modify, and/or distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
#
# THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
# WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
# ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
# WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
# ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
# OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
#

import os
import SCons.Errors

# C/C++ warning flags.
warning_flags = [
        '-Wall', '-Wextra', '-Werror', '-Wno-variadic-macros',
        '-Wno-unused-parameter', '-Wwrite-strings', '-Wmissing-declarations',
        '-Wredundant-decls', '-Wno-format',
]

# Architecture/platform support and help text.
supported_archs = {
	'x86': ('Intel/AMD x86 CPUs (includes both 32- and 64-bit support).', {
		'pc': 'Standard PCs.',
	}),

	'arm': ('ARM CPUs. Actual targeted ARM CPU determined by platform.', {
		'omap3': "Systems based on Texas Instruments' OMAP3, including the BeagleBoard.",
	})
}

# Boolean feature flags. These will be set in CPPDEFINES.
features = [
	('DEBUG', 'Compile with debugging features enabled.', True),
]

# Set up the build configuration. The no_config function is called if a required
# value is not set.
config = Variables('.config.cache')
def no_config():
	print 'Before KBoot can be built, there are certain configuration values that must be'
	print 'set. The following values are required:'
	print ''
	print ' ARCH:     Target architecture. Full list of supported architectures can be'
	print '           obtained by running: scons ARCH=?'
	print ' PLATFORM: Target platform. Full list of supported platforms can be obtained by'
	print '           running: scons ARCH=<arch> PLATFORM=?'
	print ''
	print 'To set these options you must set them on the command line when running scons.'
	print 'They will be saved so that you do not need to set them each time you build.'
	print 'For example:'
	print ''
	print ' $ scons ARCH=x86 PLATFORM=pc'
	print ''
	print 'There are other options which you can set. For a full list, run: scons -h'
	raise SCons.Errors.UserError('Required configuration values not set')

# Add the architecture configuration.
def arch_validator(key, value, env):
	if not value: no_config()
	elif value == '?':
		print 'Supported architectures:'
		for (arch, data) in supported_archs.items():
			print " %5s - %s" % (arch, data[0])
		Exit()
	elif value not in supported_archs.keys():
		raise SCons.Errors.UserError("Unsupported architecture '%s'. Use 'scons ARCH=?'." % (value))
config.Add('ARCH', 'Target architecture. Use ARCH=? for a list of possible values.', '', arch_validator)

# Add the platform configuration.
def platform_validator(key, value, env):
	if not value: no_config()
	elif value == '?':
		print 'Platforms supported by %s:' % (env['ARCH'])
		for (plat, desc) in supported_archs[env['ARCH']][1].items():
			print " %5s - %s" % (plat, desc)
		Exit()
	elif value not in supported_archs[env['ARCH']][1].keys():
		raise SCons.Errors.UserError(
			"Platform '%s' unsupported by architecture '%s'. Use 'scons PLATFORM=?'." % (
				value, env['ARCH']))
config.Add('PLATFORM', 'Target platform. Use ARCH=<arch> PLATFORM=? for a list of possible values.', '', platform_validator)

# Add boolean feature options.
for feature in features:
	config.Add(BoolVariable(feature[0], feature[1], feature[2]))

# Add other configuration variables.
config.AddVariables(
	('CROSS', 'Cross-compiler prefix. (e.g. /usr/cross/bin/i686-elf-)', ''),
	('CCFLAGS', 'Extra flags to pass to the compiler.', ''),
)

# Create the build environment and save the configuration.
env = Environment(ENV = os.environ, variables = config)
config.Save('.config.cache', env)

# Set the help text.
Help('There are a number of options available to configure the build process and the\n')
Help('features that will enabled. To change them, pass <option>=<value> to scons,\n')
Help('e.g. scons DEBUG=1\n')
Help(config.GenerateHelpText(env))

# Add a builder to preprocess linker scripts.
env['BUILDERS']['LDScript'] = Builder(action = Action(
	'$CC $_CCCOMCOM $ASFLAGS -E -x c $SOURCE | grep -v "^\#" > $TARGET',
	'$GENCOMSTR'))

# Set paths to the various build utilities. The stuff below is to support use
# of clang's static analyzer.
if os.environ.has_key('CC') and os.path.basename(os.environ['CC']) == 'ccc-analyzer':
	env['CC'] = os.environ['CC']
	env['ENV']['CCC_CC'] = env['CROSS'] + 'gcc'
else:
	env['CC'] = env['CROSS'] + 'gcc'
if os.environ.has_key('CXX') and os.path.basename(os.environ['CXX']) == 'c++-analyzer':
	env['CXX'] = os.environ['CXX']
	env['ENV']['CCC_CXX'] = env['CROSS'] + 'g++'
else:
	env['CXX'] = env['CROSS'] + 'g++'
env['AS']      = env['CROSS'] + 'as'
env['OBJDUMP'] = env['CROSS'] + 'objdump'
env['READELF'] = env['CROSS'] + 'readelf'
env['NM']      = env['CROSS'] + 'nm'
env['STRIP']   = env['CROSS'] + 'strip'
env['AR']      = env['CROSS'] + 'ar'
env['RANLIB']  = env['CROSS'] + 'ranlib'
env['OBJCOPY'] = env['CROSS'] + 'objcopy'
env['LD']      = env['CROSS'] + 'ld'

# Override default assembler - it uses as directly, we want to use GCC.
env['ASCOM'] = '$CC $_CCCOMCOM $ASFLAGS -c -o $TARGET $SOURCES'

# Set our flags in the environment.
env['CCFLAGS'] = warning_flags + ['-gdwarf-2', '-pipe', '-nostdlib', '-nostdinc',
	'-ffreestanding', '-fno-stack-protector'] + env['CCFLAGS'].split()
env['CFLAGS'] = ['-std=gnu99']
env['ASFLAGS'] = ['-D__ASM__', '-nostdinc']
env['LINKFLAGS'] = ['-nostdlib']

# Override any optimisation level specified, we want to optimise for size.
env['CCFLAGS'] = filter(lambda f: f[0:2] != '-O', env['CCFLAGS']) + ['-Os']

# Add the GCC include directory for some standard headers.
from subprocess import Popen, PIPE
incdir = Popen([env['CC'], '-print-file-name=include'], stdout=PIPE).communicate()[0].strip()
env['CCFLAGS'] += ['-isystem', incdir]
env['ASFLAGS'] += ['-isystem', incdir]

# Add definitions for feature options.
env['CPPDEFINES'] = {}
for feature in features:
	if env[feature[0]]:
		env['CPPDEFINES'][feature[0]] = 1

# Make the output nice.
verbose = ARGUMENTS.get('V') == '1'
if not verbose:
	env['ARCOMSTR']     = ' AR     $TARGET'
	env['ASCOMSTR']     = ' ASM    $SOURCE'
	env['ASPPCOMSTR']   = ' ASM    $SOURCE'
	env['CCCOMSTR']     = ' CC     $SOURCE'
	env['CXXCOMSTR']    = ' CXX    $SOURCE'
	env['LINKCOMSTR']   = ' LINK   $TARGET'
	env['RANLIBCOMSTR'] = ' RANLIB $TARGET'
	env['GENCOMSTR']    = ' GEN    $TARGET'
	env['STRIPCOMSTR']  = ' STRIP  $TARGET'

# For compatibility with the main Kiwi build system.
env['SRCARCH'] = env['ARCH']

# Change the Decider to MD5-timestamp to speed up the build a bit.
Decider('MD5-timestamp')

Export('env')
SConscript('SConscript', variant_dir = os.path.join('build', '%s-%s' % (env['ARCH'], env['PLATFORM'])))
