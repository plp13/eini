# Build file for simple example

# Compiler
cc = meson.get_compiler('c')
# Uncomment the following to expose extra warnings
# add_global_arguments(['-Wall', '-Wextra', '-Wno-sign-compare'], language: 'c')
# add_global_arguments('-O2', '-D_FORTIFY_SOURCE=2', language: 'c')

# Dependencies
deps = []
if get_option('libbsd').enabled() or get_option('libbsd').auto()
  libbsd = dependency('libbsd-overlay', required: true)
  deps += [libbsd]
endif

# Executable
t_exe = executable('example',
  sources: ['example.c', 'eini.c'],
  dependencies: deps,
  install: false
)
