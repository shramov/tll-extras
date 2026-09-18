project('tll-extras', 'c', 'cpp', version: '0.0.0', default_options: ['cpp_std=c++2a', 'werror=true', 'optimization=2'], meson_version: '>= 0.53')

include = [include_directories('src')]

tll = dependency('tll')
fmt = dependency('fmt')
zip = dependency('libzip')

lib = shared_library('tll-restore',
	['src/channel.cc'],
	include_directories : include,
	dependencies : [fmt, tll],
	install : true,
)

lib = shared_library('tll-zipconf',
	['src/zipconf.cc'],
	include_directories : include,
	dependencies : [fmt, tll, zip],
	install : true,
)

test('pytest', import('python').find_installation('python3')
	, args: ['-m', 'pytest', '-v', '--log-level=DEBUG', 'tests/']
	, env: 'BUILD_DIR=@0@'.format(meson.current_build_dir())
	, workdir: meson.current_source_dir()
)
