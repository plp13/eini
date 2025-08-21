# Build file for eINI sources

# Compiler
cc = meson.get_compiler('c')
# Uncomment the following to expose extra warnings
# add_global_arguments(['-Wall', '-Wextra', '-Wno-sign-compare'], language: 'c')
# add_global_arguments('-O2', '-D_FORTIFY_SOURCE=2', language: 'c')

# Non-main sources
src = [
  'eini.c'
]

# Dependencies
deps = []
if get_option('libbsd').enabled() or get_option('libbsd').auto()
  libbsd = dependency('libbsd-overlay', required: true)
  deps += [libbsd]
endif
if get_option('tests').enabled() or get_option('tests').auto()
  cunit = dependency('cunit', required: true)
  deps += [cunit]
endif

# Unit testing (executable and tests)
if get_option('tests').enabled()
  t_exe = executable('tests',
    sources: src + ['tests.c'],
    dependencies: deps,
    install: false
  )
  t_all = run_command(find_program('tests_list.sh'), check: true).stdout().split('\n')
  foreach t: t_all
    if t != ''
      test(t, t_exe, args: [t])
    endif
  endforeach
endif
