#!/usr/bin/env python

top = '.'
out = 'build'

import Options, os, subprocess



def check_compiler_flag(conf, flag, lang):
	return conf.check(fragment = 'int main() { float f = 4.0; char c = f; return c - 4; }\n', execute = 0, define_ret = 0, msg = 'Checking for compiler switch %s' % flag, cxxflags = conf.env[lang + 'FLAGS'] + [flag], okmsg = 'yes', errmsg = 'no')  # the code inside fragment deliberately does an unsafe implicit cast float->char to trigger a compiler warning; sometimes, gcc does not tell about an unsupported parameter *unless* the code being compiled causes a warning



def add_compiler_flags(conf, env, flags, lang, uselib = ''):
	for flag in flags:
		if type(flag) == type(()):
			flag_candidate = flag[0]
			flag_alternative = flag[1]
		else:
			flag_candidate = flag
			flag_alternative = None

		if uselib:
			flags_pattern = lang + 'FLAGS_' + uselib
		else:
			flags_pattern = lang + 'FLAGS'

		if check_compiler_flag(conf, flag_candidate, lang):
			env[flags_pattern] += [flag_candidate]
		elif flag_alternative:
			if check_compiler_flag(conf, flag_alternative, lang):
				env[flags_pattern] += [flag_alternative]



def set_options(opt):
	opt.tool_options('compiler_cc compiler_cxx gas')
	opt.tool_options('boost_', '.')
	opt.add_option('--build-variants', action='store', default='debug,release', help='build the selected variants')
	opt.add_option('--enable-extra-warnings', action='store_true', default='', help='enable warnings that are usually disabled')
	opt.recurse('src/backend')
	opt.recurse('src/common')
	opt.recurse('src/frontend')


def get_num_jobs():
	if Options.options.jobs:
		return Options.options.jobs
	else:
		return Options.default_jobs



def run_cmd(cmd, dir_):
	print "====> Executing: %s    in directory: %s" % (cmd, dir_)
	olddir = os.getcwd()
	os.chdir(dir_)
	os.waitpid(subprocess.Popen(cmd, shell=True).pid, 0)
	os.chdir(olddir)



def configure(conf):
	conf.check_tool('compiler_cc compiler_cxx gas')
	conf.check_tool('unittest')
	conf.check_tool('boost_', '.')

	conf.check_boost(lib='thread', mandatory=1)

	conf.env['BUILD_VARIANTS'] = Options.options.build_variants.split(',')

	# add common compiler flags that will be used in both variants
	add_compiler_flags(conf, conf.env, ['-Wextra', '-Wall', ('-std=c++0x', '-std=c++98'), '-pedantic'], 'CXX', 'STRICT')
	# add flags that disable some warnings (which occur in boost)
	if not Options.options.enable_extra_warnings:
		add_compiler_flags(conf, conf.env, ['-Wno-missing-field-initializers', '-Wno-long-long', '-Wno-empty-body', '-Wno-unused-parameter', '-Wno-ignored-qualifiers', '-Wno-strict-aliasing'], 'CXX', 'STRICT')

	# externals configured by waf
	conf.recurse('extern/dumb-0.9.3')
	conf.recurse('extern/Game_Music_Emu-0.5.2')

	# sources
	conf.recurse('src/backend')
	conf.recurse('src/common')
	conf.recurse('src/frontend')

	# add debug variant
	if 'debug' in conf.env['BUILD_VARIANTS']:
		dbg_env = conf.env.copy()
		dbg_env.set_variant('debug')
		conf.set_env_name('debug', dbg_env)
		add_compiler_flags(conf, dbg_env, ['-O0', '-g3', '-ggdb'], 'CC', 'BUILDMODE')
		add_compiler_flags(conf, dbg_env, ['-O0', '-g3', '-ggdb'], 'CXX', 'BUILDMODE')
		conf.write_config_header('config.h', dbg_env)

	# add release variant
	if 'release' in conf.env['BUILD_VARIANTS']:
		rel_env = conf.env.copy()
		rel_env.set_variant('release')
		conf.set_env_name('release', rel_env)
		add_compiler_flags(conf, rel_env, ['-O2', '-s', '-fomit-frame-pointer', '-pipe'], 'CC', 'BUILDMODE')
		add_compiler_flags(conf, rel_env, ['-O2', '-s', '-fomit-frame-pointer', '-pipe'], 'CXX', 'BUILDMODE')
		conf.write_config_header('config.h', rel_env)



def external(ctx):
	# mpg123
	run_cmd("./configure --enable-static --disable-shared", 'extern/mpg123-1.12.3')
	run_cmd("make -j%d" % get_num_jobs(), 'extern/mpg123-1.12.3')

	# uade
	run_cmd("./configure --without-uadefs --without-audacious --without-uade123 --without-xmms --only-uadecore", 'extern/uade-2.13')
	run_cmd("make -j%d" % get_num_jobs(), 'extern/uade-2.13')

	# adplug
	run_cmd("./configure --enable-static --disable-shared", 'extern/adplug-2.2.1')
	run_cmd("make -j%d" % get_num_jobs(), 'extern/adplug-2.2.1')



def clean_external(ctx):
	run_cmd("make distclean", 'extern/mpg123-1.12.3')
	run_cmd("make clean", 'extern/uade-2.13')
	run_cmd("make distclean", 'extern/adplug-2.2.1')



def build(bld):
	# externals built by waf
	bld.recurse('extern/dumb-0.9.3')
	bld.recurse('extern/Game_Music_Emu-0.5.2')
	bld.recurse('extern/jsoncpp')

	bld.recurse('src/backend')
	bld.recurse('src/common')
	bld.recurse('src/frontend')


	# unit test utility functions

	def find_unit_tests(unit_test_paths):
		unit_tests = []
		for unit_test_path in unit_test_paths:
			local_unit_tests = bld.path.ant_glob(unit_test_path + '/**/*.cpp')
			if local_unit_tests:
				unit_tests += local_unit_tests.split(' ') # ant_glob returns the filenames concatenated and whitespace separated, in one string
		return unit_tests

	import re
	r_test = re.compile('.cpp$')


	## misc unit tests

	unit_tests = find_unit_tests(["test/common_tests"])


	# create build task generators for all trivial unit tests
	for unit_test in unit_tests:
		bld(
			features = ['cxx', 'cprogram', 'test'],
			uselib_local = 'ionplayer_backend ionplayer_common',
			uselib = 'BOOST_THREAD BOOST BUILDMODE STRICT',
			target = r_test.sub('.test', unit_test),
			includes = '. test',
			source = unit_test
		)


	# non-trivial tests
	bld.recurse('test/scanner')


	# get the list of variants
	build_variants = bld.env['BUILD_VARIANTS']


	# duplicate task generators for the selected variant(s)
	for obj in bld.all_task_gen[:]: 
		for x in build_variants:
			cloned_obj = obj.clone(x) 
		obj.posted = True


	# add post-build function to show the unit test result summary
	import unittestw
	bld.add_post_fun(unittestw.summary)

