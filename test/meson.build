project('hydra-test', 'cpp',
  default_options: ['cpp_std=c++17'])

incdirs = include_directories('../include')

deps = []

system = host_machine.system()

if system == 'windows'
  cpp = meson.get_compiler('cpp')
  synchapi = cpp.find_library('synchronization', required:true)
  deps += [synchapi]
endif

executable('hydra-test', 'test.cpp',
	include_directories: incdirs,
	dependencies: deps)
