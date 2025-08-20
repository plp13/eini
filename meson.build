# Master build file

# Project declaration
project('eini', 'c',
  license: 'BSD-2-Clause',
  version: '1.0.0'
)

# Executable
subdir('src')

# README.md
if get_option('docs').enabled() or get_option('docs').auto()
  install_data(
    ['README.md'],
    install_dir: get_option('docdir'),
    install_tag: 'doc'
  )
endif
