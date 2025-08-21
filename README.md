# eINI
An extended .ini file parser for C

eINI was originally developed as part of [Qman](https://github.com/plp13/qman),
which at the time was using [inih](https://github.com/benhoyt/inih) for parsing
its configuration file. It provides some features that inih used to lack, namely
an `include` directive, proper .ini file path and line number reporting, and a
simpler / better documented code base.

## Features
- As simple as possible: Just add [eini.h](src/eini.h]) and [eini.c](src/eini.c)
  to your project, and you are good to go!
- Standard [.ini file](https://en.wikipedia.org/wiki/INI_file) parsing, with
  `[section]`s and `key=value` pairs
- `include` directive allows you to include additional .ini files
- Comments
- Easy error reporting: for every `key=value` pair (and every error) eINI
  provides the .ini file path and line number where it was encountered

## Limitations
- As simple as possible: eINI performs .ini file parsing, and nothing else.
  Locating your .ini files, parsing the contents of values, storing results into
  memory, etc. are not within scope.
- Unlike inih, eINI is not customizable

## Simple usage example
In this example we'll parse the following .ini files:

*config.ini*
```
; Test .ini file

[colors]
foreground = red    ; foreground color
background = blue   ; background color

[layout]
margin_width = 8    ; margin width (in characters)

include other.ini

; The following will cause an error
include does_not_exist.ini
```

*other.ini*
```
; Included by config.ini

[master]
greeting = "Hello, world!"
```

Copy [eini.h](src/eini.h) and [eini.c](src/eini.c) to your working directory,
and type the following program:

*example.c*
```c
// eINI simple usage example

#include <stdio.h>
#include <stdlib.h>

#include "eini.h"

// Handler function. For this test, we just define a function that prints what
// it gets. In real life, you'll want one that performs error checking and
// updates your program's configuration.
void handler_func(const wchar_t *section, const wchar_t *key,
                  const wchar_t *value, const char *path, const unsigned line) {
  printf("Line %d of %s: Got %ls.%ls='%ls'\n", line, path, section, key, value);
}

// Error function. For this test, we just define a function that prints the
// error message.
void error_func(const wchar_t *error, const char *path, const unsigned line) {
  printf("Error in %s line %d: %ls\n", path, line, error);
}

int main(int argc, char **argv) {
  // User must provide the .ini file to parse as an argument
  if (1 == argc) {
    printf("Usage: %s <.ini file>\n", argv[0]);
    exit(-1);
  }

  // Initialize eINI
  eini_init();

  // Parse
  eini(handler_func, error_func, argv[1]);

  // Wind down eINI and exit
  eini_winddown();
  exit(0);
}
```

The example program can now be compiled and executed:

```
$ gcc -o example example.c eini.c
$ ./example config.ini
Line 4 of config.ini: Got colors.foreground='red'
Line 5 of config.ini: Got colors.background='blue'
Line 8 of config.ini: Got layout.margin_width='8'
Line 4 of ./other.ini: Got master.greeting='Hello, world!'
Error in config.ini line 13: Unable to open './does_not_exist.ini'
```

The code for this example, together with a `meson` build file for it, can be
found in [examples/simple](examples/simple).

## Technical notes
- Parsing is done one line at a time
- There are just 4 syntax elements:
  - **Comments** begin with a `;` and end with a newline. Multi-line comments
    are not supported.
  - **Sections** are declared using brackets, e.g. `[section]`. Section names
    must be identifiers, i.e. they must begin with a letter and must only include
    letters, numbers, or the underscore character (`_`).
  - **Key/value pairs** are declared using the `key=value` syntax. Keys must be
    identifiers.
  - **Include directives** use the `include /path/to/file.ini` syntax to
    instruct eINI to parse another .ini file
- Values and file paths are strings. They can optionally be quoted using single
  (`'`) or double quotes (`"`), and can also include the following escape
  sequences:
  - `\a`, `\b`, `\t`, `\n`, `\v`, `\f`, and `\r` – interpreted according to the
    ASCII standard
  - `\e` – interpreted as an escape (`0x1b`) character
  - `\\` – interpreted as a backslash literal
  - `\'` - interpreted as a signle quote literal
  - `\"` - interpreted as a double quote literal
- Paths are relative to the base directory of the .ini file that performs the
  inclusion. For example if you `include themes/modern.ini` from
  `/etc/xdg/program/main.conf`, `modern.ini` is expected to be found in
  `/etc/xdg/program/themes/`. Absolute paths can also be used.
