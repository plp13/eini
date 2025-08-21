// eINI (definition)

#ifndef EINI_H

#define EINI_H

#include <regex.h>
#include <stddef.h>

//
// Types
//

// Boolean
#ifndef bool
#define bool int
#endif

// Type of a .ini file line
typedef enum {
  EINI_ERROR,   // parse error
  EINI_NONE,    // line is empty
  EINI_INCLUDE, // line contains an include directive
  EINI_SECTION, // line contains a section header
  EINI_VALUE    // line contains a key/value pair
} eini_type_t;

// Parsed contents of a line in a .ini file
typedef struct {
  eini_type_t type; // line type
  wchar_t *key;     // key name, if `type` is `EINI_VALUE`
                    // NULL, otherwise
  wchar_t *value;   // error message, if type is `EINI_ERROR`
                    // path of file to include, if type is `EINI_INCLUDE`
                    // section name, if type is `EINI_SECTION`
                    // value, if type is `EINI_VALUE`
                    // NULL, otherwise
} eini_t;

// Handler function
typedef void (*eini_handler_t)(const wchar_t *section, // current section name
                               const wchar_t *key,     // key name
                               const wchar_t *value,   // value
                               const char *path,       // .ini file path
                               const unsigned line     // .ini file line
);

// Error handler function
typedef void (*eini_error_t)(const wchar_t *error, // error message
                             const char *path,     // .ini file path
                             const unsigned line   // .ini file line
);

//
// Constants
//

// Boolean values
#ifndef true
#define true 1
#endif
#ifndef false
#define false 0
#endif

// Buffer sizes
#define EINI_SHORT 128 // length of a short array (suitable for a token)
#define EINI_LONG 1024 // length of a longer array (suitable for a line of text)

//
// Global variables
//

// Regular expressions for...
extern regex_t eini_re_include, // an include directive
    eini_re_section,            // a section header
    eini_re_value;              // a key/value pair

//
// Functions
//

// Initialize eINI
extern void eini_init();

// Parse .ini file line in `src` and return the result
extern eini_t eini_parse(char *src);

// Parse .ini file in `path`, calling `hf()` whenever a key/value pair is found,
// or `ef()` in case of error. `eini()` will recursively call itself whenever it
// encounters an `include` directive.
extern void eini(eini_handler_t hf, eini_error_t ef, const char *path);

// Wind down eINI
extern void eini_winddown();

#endif
