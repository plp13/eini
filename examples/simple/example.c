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
