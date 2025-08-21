// eINI (implementation)

#include <alloca.h>
#include <errno.h>
#include <libgen.h>
#include <regex.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <wchar.h>
#include <wctype.h>

#include "eini.h"

//
// Types
//

// A range
typedef struct {
  unsigned beg; // beginning
  unsigned end; // end
} range_t;

//
// Global variables
//

regex_t eini_re_include, eini_re_section, eini_re_value;

//
// Helper functions and macros
//

// Helper of `wsrc_strip` and `eini_parse()`. Set the `type`, `key`, and `value`
// fields of `ret` to `mytype`, `mykey`, and `myvalue` respectively.
#define set_ret(mytype, mykey, myvalue)                                        \
  ret.type = mytype;                                                           \
  if (NULL == mykey)                                                           \
    ret.key = NULL;                                                            \
  else {                                                                       \
    wcslcpy(ret_key, NULL != mykey ? mykey : L"", EINI_SHORT);                 \
    ret.key = ret_key;                                                         \
  }                                                                            \
  if (NULL == myvalue)                                                         \
    ret.value = NULL;                                                          \
  else {                                                                       \
    wcslcpy(ret_value, NULL != myvalue ? myvalue : L"", EINI_LONG);            \
    wunescape(ret_value);                                                      \
    ret.value = ret_value;                                                     \
  }

// Helper of `eini_parse()`. Strip trailing whitespace from `wsrc`. If it's
// surrounded by single or double quotes, strip those as well. Return any syntax
// errors pertaining to non-terminated quotes.
#define wsrc_strip                                                             \
  wlen = wmargtrim(wsrc, NULL);                                                \
  if (L'"' == wsrc[0]) {                                                       \
    /* wsrc begins with a `"` */                                               \
    if (wlen < 2) {                                                            \
      /* wsrc equals `"`; accept it as it is */                                \
    } else if (L'"' == wsrc[wlen - 1]) {                                       \
      /* wsrc ends in '"' */                                                   \
      if (wescaped(wsrc, wlen - 1)) {                                          \
        /* the ending `"` is escaped; reject it */                             \
        set_ret(EINI_ERROR, NULL, L"Non-terminated quote");                    \
        return ret;                                                            \
      } else {                                                                 \
        /* the ending `"` is not escaped; remove the `"`s and accept it */     \
        wsrc[wlen - 1] = L'\0';                                                \
        wsrc = &wsrc[1];                                                       \
      }                                                                        \
    } else {                                                                   \
      /* wsrc does not end in `"`; reject it */                                \
      set_ret(EINI_ERROR, NULL, L"Non-terminated quote");                      \
      return ret;                                                              \
    }                                                                          \
  } else if (L'\'' == wsrc[0]) {                                               \
    /* wsrc begins with a `'` */                                               \
    if (wlen < 2) {                                                            \
      /* wsrc equals `'`; accept it as it is */                                \
    } else if (L'\'' == wsrc[wlen - 1]) {                                      \
      /* wsrc ends in `'` */                                                   \
      if (wescaped(wsrc, wlen - 1)) {                                          \
        /* the ending `'` is escaped; reject it */                             \
        set_ret(EINI_ERROR, NULL, L"Non-terminated quote");                    \
        return ret;                                                            \
      } else {                                                                 \
        /* the ending `"` is not escaped; remove the `"`s and accept it */     \
        wsrc[wlen - 1] = L'\0';                                                \
        wsrc = &wsrc[1];                                                       \
      }                                                                        \
    } else {                                                                   \
      /* wsrc does not end in `'`; reject it */                                \
      set_ret(EINI_ERROR, NULL, L"Non-terminated quote");                      \
      return ret;                                                              \
    }                                                                          \
  }

// Helper of `populate_ipath` and `eini()`. Call `ef(errmsg, path, i)`, wind
// down, and return.
#define call_ef_and_return                                                     \
  ef(errmsg, path, i);                                                         \
  if (NULL != fp)                                                              \
    fclose(fp);                                                                \
  return;

// Helper of `eini()`, called when handling an inclusion. Populate `ipath` with
// the correct path of the included file. In case of error (such as file not
// found), call `em()` and return.
#define populate_ipath                                                         \
  if (L'/' == lne.value[0]) {                                                  \
    /* Absolute path */                                                        \
    if (-1 == wcstombs(ipath, lne.value, EINI_LONG)) {                         \
      wcslcpy(errmsg, L"wcstombs() failed", EINI_LONG);                        \
      call_ef_and_return;                                                      \
    }                                                                          \
  } else {                                                                     \
    /* Relative path */                                                        \
    char apath[EINI_LONG];                                                     \
    if (-1 == wcstombs(apath, lne.value, EINI_LONG)) {                         \
      wcslcpy(errmsg, L"wcstombs() failed", EINI_LONG);                        \
      call_ef_and_return;                                                      \
    }                                                                          \
    strlcpy(ipath, dirname(ipath), EINI_LONG);                                 \
    strlcat(ipath, "/", EINI_LONG);                                            \
    strlcat(ipath, apath, EINI_LONG);                                          \
  }                                                                            \
  /* Error out if file was not found */                                        \
  ifp = fopen(ipath, "r");                                                     \
  if (NULL == ifp) {                                                           \
    swprintf(errmsg, EINI_LONG, L"Unable to open '%s'", ipath);                \
    call_ef_and_return;                                                        \
  }                                                                            \
  fclose(ifp);

// Helper of `wsrc_strip()` and `decomment()`, i.e. `eini_parse()` ultimately.
// Test whether the character in `src[pos]` is escaped.
bool wescaped(wchar_t *src, unsigned pos) {
  int c = 0;   // number of '\'s before `pos`
  int i = pos; // iterator

  while (i > 0) {
    i--;
    if (L'\\' == src[i])
      c++;
    else
      break;
  }

  return c % 2;
}

// Helper of `set_ret()`, i.e. `eini_parse()` ultimately. Unescape the string in
// `src`. `\a`, `\b`, `\t`, `\n`, `\v`, `\f`, and `\r` are unescaped into
// character codes 7 to 13, `\e` to character code 27 (ESC), `\\` to `\`, `\"`
// to `"`, and `\'` to `'`. All other escaped characters are discarded.
void wunescape(wchar_t *src) {
  int i = 0, j = 0; // iterators

  while (L'\0' != src[i]) {
    if (L'\\' == src[i]) {
      switch (src[i + 1]) {
      case L'a':
        src[j++] = L'\a';
        break;
      case L'b':
        src[j++] = L'\b';
        break;
      case L't':
        src[j++] = L'\t';
        break;
      case L'n':
        src[j++] = L'\n';
        break;
      case L'v':
        src[j++] = L'\v';
        break;
      case L'f':
        src[j++] = L'\f';
        break;
      case L'r':
        src[j++] = L'\r';
        break;
      case L'e':
        src[j++] = L'\e';
        break;
      case L'\\':
        src[j++] = L'\\';
        break;
      case L'"':
        src[j++] = L'"';
        break;
      case L'\'':
        src[j++] = L'\'';
        break;
      }
      i += 2;
    } else {
      src[j++] = src[i];
      i++;
    }
  }

  src[j] = L'\0';
}

// Helper of `eini_parse()`. Return the position of the first character in `src`
// that is not whitespace, and not one of the characters in `extras`. (If
// `extras` is NULL then it is ignored.)
unsigned wmargend(const wchar_t *src, const wchar_t *extras) {
  bool ws;       // whether current character is whitespace or in extras
  unsigned i, j; // iterators

  if (NULL == extras)
    extras = L"";

  for (i = 0; L'\0' != src[i]; i++) {
    ws = false;
    if (iswspace(src[i]))
      ws = true;
    for (j = 0; L'\0' != extras[j]; j++)
      if (src[i] == extras[j])
        ws = true;

    if (!ws)
      return i;
  }

  return 0;
}

// Helper of `wsrc_strip` and `eini_parse()`. Trim all characters at the end of
// `trgt` that are either whitespace or one of the charactes in `extras`. (If
// `extras` is NULL then it is ignored.) Trimming is done by inserting 0 or more
// NULL characters at the end of `trgt`. Return the new length of `trgt`.
unsigned wmargtrim(wchar_t *trgt, const wchar_t *extras) {
  int i;      // iterator
  unsigned j; // iterator
  bool trim;  // true if we'll be trimming `trgt[i]`

  if (NULL == extras)
    extras = L"";

  for (i = 0; L'\0' != trgt[i]; i++)
    ;

  i--;
  while (i >= 0) {
    trim = false;

    if (iswspace(trgt[i]))
      trim = true;
    else
      for (j = 0; L'\0' != extras[j]; j++)
        if (trgt[i] == extras[j])
          trim = true;

    if (!trim) {
      trgt[i + 1] = L'\0';
      return i + 1;
    }

    i--;
  }

  trgt[0] = L'\0';
  return 0;
}

// Helper of `eini_parse()`. Discard all comments in `src`.
void decomment(wchar_t *src) {
  unsigned i = 0;       // iterator
  bool inq = false;     // true if `src[i]` is inside a quoted string
  wchar_t qtype = L'?'; // quote type: `'` or `"`

  while (L'\0' != src[i]) {
    if (inq) {
      // `src[i]` is inside a quoted string
      if (qtype == src[i] && !wescaped(src, i)) {
        // `src[i]` is an unescaped `"` or `'`, thus the quoted string ends
        inq = false;
      }
    } else {
      // `src[i]` isn't inside a quoted string
      if (L';' == src[i]) {
        // `src[i]` is `;`, thus we have a comment to the end of line
        src[i] = L'\0';
        return;
      } else if (L'"' == src[i] && !wescaped(src, i)) {
        // `src[i]` is an unescaped `"`, thus a double-quoted string begins
        inq = true;
        qtype = L'"';
      } else if (L'\'' == src[i] && !wescaped(src, i)) {
        // src[i] is an unescaped `'`, thus a single-quoted string begins
        inq = true;
        qtype = L'\'';
      }
    }
    i++;
  }
}

// Helper of `eini_parse()`. Find the first match of `re` in `src` and return
// its location.
range_t match(regex_t re, char *src) {
  regmatch_t pmatch[1]; // regex match
  range_t res;          // return value

  // Try to match `src`
  int err = regexec(&re, src, 1, pmatch, 0);

  if (0 == err) {
    // A match was found
    res.beg = pmatch[0].rm_so;
    res.end = pmatch[0].rm_eo;
  } else {
    // No match found, or an error has occured; return `{0, 0}`
    res.beg = 0;
    res.end = 0;
  }

  return res;
}

//
// Functions
//

void eini_init() {
  regcomp(&eini_re_include, "[[:space:]]*include[[:space:]]*", REG_EXTENDED);
  regcomp(&eini_re_section,
          "[[:space:]]*\\[[[:space:]]*[a-zA-Z][a-zA-Z0-9_]*[[:space:]]*\\][[:"
          "space:]]*",
          REG_EXTENDED);
  regcomp(&eini_re_value,
          "[[:space:]]*[a-zA-Z][a-zA-Z0-9_]*[[:space:]]*=[[:space:]]*",
          REG_EXTENDED);
}

eini_t eini_parse(char *src) {
  wchar_t *wsrc =
      alloca((EINI_LONG + 1) * sizeof(wchar_t)); // `wchar_t*` version of `src`
  char *csrc = alloca(
      (EINI_LONG + 1) *
      sizeof(char)); // `char*` version of `src` (after removing comments)
  int wlen;          // length of `wsrc`
  range_t loc;       // location of regex match in `wsrc`
  eini_t ret;        // return value
  static wchar_t ret_key[EINI_SHORT];  // `key` contents of `ret`
  static wchar_t ret_value[EINI_LONG]; // `value` contents of `ret`

  wlen = mbstowcs(wsrc, src, EINI_LONG);
  if (-1 == wlen) {
    // Couldn't convert `src`
    set_ret(EINI_ERROR, NULL, L"Non-string data");
    return ret;
  }
  wsrc[EINI_LONG - 1] = L'\0';

  decomment(wsrc);
  wcstombs(csrc, wsrc, EINI_LONG);

  loc = match(eini_re_include, csrc);

  if (0 == loc.beg && loc.end > loc.beg) {
    // Include directive
    wsrc = &wsrc[loc.end];
    wsrc_strip;
    set_ret(EINI_INCLUDE, NULL, wsrc);
    return ret;
  }

  loc = match(eini_re_section, csrc);
  if (!(0 == loc.beg && 0 == loc.end)) {
    // Section
    wsrc = &wsrc[wmargend(wsrc, L"[")];
    wmargtrim(wsrc, L"]");
    set_ret(EINI_SECTION, NULL, wsrc);
    return ret;
  }

  loc = match(eini_re_value, csrc);
  if (!(0 == loc.beg && 0 == loc.end)) {
    // Key/value pair
    wchar_t *wkey = wsrc;
    wkey[loc.end - 1] = L'\0';
    wkey = &wkey[wmargend(wkey, NULL)];
    wmargtrim(wkey, L"=");
    wsrc = &wsrc[loc.end];
    wsrc_strip;
    set_ret(EINI_VALUE, wkey, wsrc);
    return ret;
  }

  wlen = wmargtrim(wsrc, NULL);
  if (0 == wlen) {
    // Empty line
    set_ret(EINI_NONE, NULL, NULL);
    return ret;
  }

  // None of the above
  wchar_t errmsg[EINI_LONG];
  swprintf(errmsg, EINI_LONG, L"Unable to parse '%ls'", wsrc);
  set_ret(EINI_ERROR, NULL, errmsg);
  return ret;
}

void eini(eini_handler_t hf, eini_error_t ef, const char *path) {
  unsigned i = 0;                // current line number in .ini file
  FILE *fp = NULL;               // file pointer for .ini file
  char ln[EINI_LONG];            // current .ini file line text
  eini_t lne;                    // current .ini file line parsed contents
  wchar_t sec[EINI_SHORT] = L""; // current section
  wchar_t errmsg[EINI_LONG];     // error message

  fp = fopen(path, "r");
  if (NULL == fp) {
    swprintf(errmsg, EINI_LONG, L"Unable to open '%s'", path);
    call_ef_and_return;
  }

  // If this is an empty file, do nothing
  fseek(fp, 0, SEEK_END);
  if (0 == ftell(fp))
    return;
  rewind(fp);

  while (!feof(fp)) {
    // Read next line into `ln`, and parse it into `lne`
    if (NULL == fgets(ln, EINI_LONG, fp)) {
      wcslcpy(errmsg, L"fgets() failed", EINI_LONG);
      call_ef_and_return;
    }
    i++;
    lne = eini_parse(ln);

    // Call `hf()` or `ef()`, depending on what we got
    switch (lne.type) {
    case EINI_INCLUDE: {
      char ipath[EINI_LONG]; // include file path
      FILE *ifp;             // include file pointer
      populate_ipath;
      eini(hf, ef, ipath);
      break;
    }
    case EINI_SECTION: {
      wcslcpy(sec, lne.value, EINI_SHORT);
      break;
    }
    case EINI_VALUE: {
      if (0 == wcslen(sec)) {
        swprintf(errmsg, EINI_LONG, L"Option '%ls' does not have a section",
                 lne.key);
        call_ef_and_return;
      } else
        hf(sec, lne.key, lne.value, path, i);
      break;
    }
    case EINI_ERROR: {
      wcslcpy(errmsg, lne.value, EINI_LONG);
      call_ef_and_return;
      break;
    }
    case EINI_NONE:
    default:
      break;
    }
  }

  fclose(fp);
}

void eini_winddown() {
  regfree(&eini_re_include);
  regfree(&eini_re_section);
  regfree(&eini_re_value);
}
