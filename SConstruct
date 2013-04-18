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

# C/C++ warning flags.
warning_flags = [
    '-Wall', '-Wextra', '-Wno-variadic-macros', '-Wno-unused-parameter',
    '-Wwrite-strings', '-Wmissing-declarations', '-Wredundant-decls',
    '-Wno-format', '-Wimplicit-function-declaration'
]

# GCC-specific warning flags.
gcc_warning_flags = ['-Wno-unused-but-set-variable']

import os, sys, SCons.Errors
from subprocess import Popen, PIPE

sys.path = [os.path.abspath(os.path.join('utilities', 'build'))] + sys.path
from kconfig import ConfigParser
import util

# Create the configuration parser.
config = ConfigParser('.config')
Export('config')

# Create the top-level build environment.
env = Environment(ENV = os.environ)

# Override default assembler - it uses as directly, we want to use GCC.
env['ASCOM'] = '$CC $_CCCOMCOM $ASFLAGS -c -o $TARGET $SOURCES'

# Make the output nice.
verbose = ARGUMENTS.get('V') == '1'
if not verbose:
    def compile_str(msg, name):
        return ' \033[0;32m%-6s\033[0m %s' % (msg, name)
    env['ARCOMSTR']     = compile_str('AR', '$TARGET')
    env['ASCOMSTR']     = compile_str('ASM', '$SOURCE')
    env['ASPPCOMSTR']   = compile_str('ASM', '$SOURCE')
    env['CCCOMSTR']     = compile_str('CC', '$SOURCE')
    env['CXXCOMSTR']    = compile_str('CXX', '$SOURCE')
    env['LINKCOMSTR']   = compile_str('LINK', '$TARGET')
    env['RANLIBCOMSTR'] = compile_str('RANLIB', '$TARGET')
    env['GENCOMSTR']    = compile_str('GEN', '$TARGET')
    env['STRIPCOMSTR']  = compile_str('STRIP', '$TARGET')

# Add a builder to preprocess linker scripts.
env['BUILDERS']['LDScript'] = Builder(action = Action(
    '$CC $_CCCOMCOM $ASFLAGS -E -x c $SOURCE | grep -v "^\#" > $TARGET',
    '$GENCOMSTR'))

# Set up the host build environment. We don't build C code with normal warning
# flags here as Kconfig won't compile with them.
host_env = env.Clone()
host_env['CCFLAGS'] = ['-pipe']
host_env['CFLAGS'] = ['-std=gnu99']

# Don't use the Default function for compatibility with the main Kiwi
# build system.
defaults = []

# Build host system utilities.
SConscript('SConscript', variant_dir = os.path.join('build', 'host'),
    exports = {'env': host_env, 'dirs': ['utilities'], 'defaults': defaults})

# Add targets to run the configuration interface.
Alias('config', host_env.ConfigMenu('__config', ['Kconfig']))

# Only do the rest of the build if the configuration exists.
if not config.configured() or 'config' in COMMAND_LINE_TARGETS:
    if GetOption('help') or 'config' in COMMAND_LINE_TARGETS:
        Return()
    else:
        raise SCons.Errors.StopError(
            "Configuration missing or out of date. Please update using 'config' target.")

# Detect which compiler to use.
compilers = ['cc', 'gcc', 'clang']
compiler = None
for name in compilers:
    path = config['CROSS_COMPILER'] + name
    if util.which(path):
        compiler = path
        break
if not compiler:
    raise SCons.Errors.StopError("Toolchain has no usable compiler available.")

# Set paths to the various build utilities. The stuff below is to support use
# of clang's static analyzer.
if os.environ.has_key('CC') and os.path.basename(os.environ['CC']) == 'ccc-analyzer':
    env['CC'] = os.environ['CC']
    env['ENV']['CCC_CC'] = compiler
else:
    env['CC'] = compiler
env['AS']      = config['CROSS_COMPILER'] + 'as'
env['OBJDUMP'] = config['CROSS_COMPILER'] + 'objdump'
env['READELF'] = config['CROSS_COMPILER'] + 'readelf'
env['NM']      = config['CROSS_COMPILER'] + 'nm'
env['STRIP']   = config['CROSS_COMPILER'] + 'strip'
env['AR']      = config['CROSS_COMPILER'] + 'ar'
env['RANLIB']  = config['CROSS_COMPILER'] + 'ranlib'
env['OBJCOPY'] = config['CROSS_COMPILER'] + 'objcopy'
env['LD']      = config['CROSS_COMPILER'] + 'ld'

# Set our flags in the environment.
env['CCFLAGS'] = warning_flags + ['-gdwarf-2', '-pipe', '-nostdlib', '-nostdinc',
    '-ffreestanding', '-fno-stack-protector'] + config['EXTRA_CCFLAGS'].split()
env['CFLAGS'] = ['-std=gnu99']
env['ASFLAGS'] = ['-D__ASM__', '-nostdinc']
env['LINKFLAGS'] = ['-nostdlib']

# Add compiler-specific flags.
output = Popen([compiler, '--version'], stdout=PIPE).communicate()[0].strip()
is_clang = output.find('clang') >= 0
if is_clang:
    # Clang's integrated assembler doesn't support 16-bit code.
    env['ASFLAGS'] += ['-no-integrated-as']
else:
    env['CCFLAGS'] += gcc_warning_flags

# Override any optimisation level specified, we want to optimise for size.
env['CCFLAGS'] = filter(lambda f: f[0:2] != '-O', env['CCFLAGS']) + ['-Os']

# Add the GCC include directory for some standard headers.
incdir = Popen([env['CC'], '-print-file-name=include'], stdout=PIPE).communicate()[0].strip()
env['CCFLAGS'] += ['-isystem', incdir]
env['ASFLAGS'] += ['-isystem', incdir]

# Change the Decider to MD5-timestamp to speed up the build a bit.
Decider('MD5-timestamp')

SConscript('SConscript', variant_dir = os.path.join('build', '%s-%s' % (config['ARCH'], config['PLATFORM'])),
    exports = {'env': env, 'dirs': ['source', 'test'], 'defaults': defaults})

Default(defaults)
