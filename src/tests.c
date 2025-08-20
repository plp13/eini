// Unit testing

// To create a test named `foo`:
// - Define your test function `void test_foo()`
// - Insert `add_test(foo)` into `main()`
//
// Invoking `qman_tests <test name>` runs a specific test, while `qman_tests
// all` runs all tests.
//
// Exit codes:
//           0: all tests succeeded
//           n: n failures happened during testing
//          -1: test not found
//
// Invoking `qman_tests all` runs all tests, and returns the number of failures
// in its exit code.

#include <CUnit/Basic.h>
#include <CUnit/CUnit.h>
#include <locale.h>
#include <stdlib.h>
#include <unistd.h>
#include <wchar.h>

#include "eini.h"

//
// Helper macros
//

// Initialize the test suite
#define init_test_suite()                                                      \
  bool test_found = false;                                                     \
  int errors = 0;                                                              \
  CU_pSuite suite;                                                             \
  if (argc > 1) {                                                              \
    printf("Running: ");                                                       \
    CU_initialize_registry();                                                  \
    suite = CU_add_suite("qman", NULL, NULL);                                  \
  }

// Add test `tst`. The name of the test function must be `test_<tst>`.
#define add_test(tst)                                                          \
  if (argc > 1) {                                                              \
    if (0 == strcmp(argv[1], #tst) || 0 == strcmp(argv[1], "all")) {           \
      test_found = true;                                                       \
      printf(#tst "\n         ");                                              \
      CU_add_test(suite, #tst, test_##tst);                                    \
    }                                                                          \
  }

// Depending on `argc`/`argv`, run one test, or run all tests, or print usage
// information. Then exit.
#define run_tests_and_exit()                                                   \
  if (argc > 1) {                                                              \
    if (test_found) {                                                          \
      printf("\n");                                                            \
      CU_basic_run_tests();                                                    \
      errors = CU_get_number_of_failures();                                    \
      CU_cleanup_registry();                                                   \
      exit(errors);                                                            \
    } else {                                                                   \
      printf("No such test '%s'\n", argv[1]);                                  \
      exit(-1);                                                                \
    }                                                                          \
  } else {                                                                     \
    printf("Usage: %s <test name>  # run a single test\n", argv[0]);           \
    printf("       %s all          # run all tests\n", argv[0]);               \
    exit(0);                                                                   \
  }

//
// Test functions
//

// Tests for `eini_parse()`

// Main test function
void test_eini_parse() {
  eini_t parsed; // return value of `eini_parse()`

  eini_init();

  parsed = eini_parse("include /usr/share/foo");
  CU_ASSERT_EQUAL(parsed.type, EINI_INCLUDE);
  CU_ASSERT(0 == wcscmp(parsed.value, L"/usr/share/foo"));

  parsed = eini_parse("\t include\t\t/usr/share/foo ");
  CU_ASSERT_EQUAL(parsed.type, EINI_INCLUDE);
  CU_ASSERT(0 == wcscmp(parsed.value, L"/usr/share/foo"));

  parsed = eini_parse("include \"/usr/share/foo\"");
  CU_ASSERT_EQUAL(parsed.type, EINI_INCLUDE);
  CU_ASSERT(0 == wcscmp(parsed.value, L"/usr/share/foo"));

  parsed = eini_parse(" include \t  \'/usr/share/foo\'  \t ");
  CU_ASSERT_EQUAL(parsed.type, EINI_INCLUDE);
  CU_ASSERT(0 == wcscmp(parsed.value, L"/usr/share/foo"));

  parsed = eini_parse("include /usr/share/foo ; comment");
  CU_ASSERT_EQUAL(parsed.type, EINI_INCLUDE);
  CU_ASSERT(0 == wcscmp(parsed.value, L"/usr/share/foo"));

  parsed = eini_parse("include \"/usr/share/foo\" ; comment");
  CU_ASSERT_EQUAL(parsed.type, EINI_INCLUDE);
  CU_ASSERT(0 == wcscmp(parsed.value, L"/usr/share/foo"));

  parsed = eini_parse("include \'/usr/share/foo ; comment\'");
  CU_ASSERT_EQUAL(parsed.type, EINI_INCLUDE);
  CU_ASSERT(0 == wcscmp(parsed.value, L"/usr/share/foo ; comment"));

  parsed = eini_parse("[section_one]");
  CU_ASSERT_EQUAL(parsed.type, EINI_SECTION);
  CU_ASSERT(0 == wcscmp(parsed.value, L"section_one"));

  parsed = eini_parse("  [\tSectionTwo  ]\t\t \n");
  CU_ASSERT_EQUAL(parsed.type, EINI_SECTION);
  CU_ASSERT(0 == wcscmp(parsed.value, L"SectionTwo"));

  parsed = eini_parse("\t [ Section3  ] \t");
  CU_ASSERT_EQUAL(parsed.type, EINI_SECTION);
  CU_ASSERT(0 == wcscmp(parsed.value, L"Section3"));

  parsed = eini_parse("[SectionIV]; comment");
  CU_ASSERT_EQUAL(parsed.type, EINI_SECTION);
  CU_ASSERT(0 == wcscmp(parsed.value, L"SectionIV"));

  parsed = eini_parse("key_1=an egg");
  CU_ASSERT_EQUAL(parsed.type, EINI_VALUE);
  CU_ASSERT(0 == wcscmp(parsed.key, L"key_1"));
  CU_ASSERT(0 == wcscmp(parsed.value, L"an egg"));

  parsed = eini_parse("\t  KeyTwo = another eggie  \t ");
  CU_ASSERT_EQUAL(parsed.type, EINI_VALUE);
  CU_ASSERT(0 == wcscmp(parsed.key, L"KeyTwo"));
  CU_ASSERT(0 == wcscmp(parsed.value, L"another eggie"));

  parsed = eini_parse(
      "\t third_key =  ένα αυγουλάκι που in English το λένε eggie  \n");
  CU_ASSERT_EQUAL(parsed.type, EINI_VALUE);
  CU_ASSERT(0 == wcscmp(parsed.key, L"third_key"));
  CU_ASSERT(
      0 == wcscmp(parsed.value, L"ένα αυγουλάκι που in English το λένε eggie"));

  parsed = eini_parse("key4= \"ακόμη ένα eggie ή αυγό\"");
  CU_ASSERT_EQUAL(parsed.type, EINI_VALUE);
  CU_ASSERT(0 == wcscmp(parsed.key, L"key4"));
  CU_ASSERT(0 == wcscmp(parsed.value, L"ακόμη ένα eggie ή αυγό"));

  parsed =
      eini_parse("key5=ASCII specials are \\a \\b \\t \\n \\v \\f \\r and \\e");
  CU_ASSERT_EQUAL(parsed.type, EINI_VALUE);
  CU_ASSERT(0 == wcscmp(parsed.key, L"key5"));
  CU_ASSERT(0 == wcscmp(parsed.value,
                        L"ASCII specials are \a \b \t \n \v \f \r and \e"));

  parsed = eini_parse("key_VI=Other specials: \\\\ \\' and \\\", naturally");
  CU_ASSERT_EQUAL(parsed.type, EINI_VALUE);
  CU_ASSERT(0 == wcscmp(parsed.key, L"key_VI"));
  CU_ASSERT(0 ==
            wcscmp(parsed.value, L"Other specials: \\ ' and \", naturally"));

  parsed = eini_parse("se7en=\"in double \\\" quotes\\r\"");
  CU_ASSERT_EQUAL(parsed.type, EINI_VALUE);
  CU_ASSERT(0 == wcscmp(parsed.key, L"se7en"));
  CU_ASSERT(0 == wcscmp(parsed.value, L"in double \" quotes\r"));

  parsed = eini_parse("se7enUp=  \'in single \\' quotes\\\\\'");
  CU_ASSERT_EQUAL(parsed.type, EINI_VALUE);
  CU_ASSERT(0 == wcscmp(parsed.key, L"se7enUp"));
  CU_ASSERT(0 == wcscmp(parsed.value, L"in single \' quotes\\"));

  parsed = eini_parse("");
  CU_ASSERT_EQUAL(parsed.type, EINI_NONE);

  parsed = eini_parse("[garbled$ect_on]");
  CU_ASSERT_EQUAL(parsed.type, EINI_ERROR);
  CU_ASSERT(0 == wcscmp(parsed.value, L"Unable to parse '[garbled$ect_on]'"));

  parsed = eini_parse("κλειδί=value");
  CU_ASSERT_EQUAL(parsed.type, EINI_ERROR);
  CU_ASSERT(0 == wcscmp(parsed.value, L"Unable to parse 'κλειδί=value'"));

  parsed = eini_parse("key=\"value"); // "value
  CU_ASSERT_EQUAL(parsed.type, EINI_ERROR);
  CU_ASSERT(0 == wcscmp(parsed.value, L"Non-terminated quote"));

  parsed = eini_parse("key=\"value\\\""); // "value\"
  CU_ASSERT_EQUAL(parsed.type, EINI_ERROR);
  CU_ASSERT(0 == wcscmp(parsed.value, L"Non-terminated quote"));

  parsed = eini_parse("key=\"value\\\\\""); // "value\\"
  CU_ASSERT_EQUAL(parsed.type, EINI_VALUE);
  CU_ASSERT(0 == wcscmp(parsed.key, L"key"));
  CU_ASSERT(0 == wcscmp(parsed.value, L"value\\"));

  parsed = eini_parse("key=\"value\\\\\\\""); // "value\\\"
  CU_ASSERT_EQUAL(parsed.type, EINI_ERROR);
  CU_ASSERT(0 == wcscmp(parsed.value, L"Non-terminated quote"));

  parsed = eini_parse("key='value"); // 'value
  CU_ASSERT_EQUAL(parsed.type, EINI_ERROR);
  CU_ASSERT(0 == wcscmp(parsed.value, L"Non-terminated quote"));

  parsed = eini_parse("key='value\\'"); // 'value\'
  CU_ASSERT_EQUAL(parsed.type, EINI_ERROR);
  CU_ASSERT(0 == wcscmp(parsed.value, L"Non-terminated quote"));

  parsed = eini_parse("key='value\\\\'"); // 'value\\'
  CU_ASSERT_EQUAL(parsed.type, EINI_VALUE);
  CU_ASSERT(0 == wcscmp(parsed.key, L"key"));
  CU_ASSERT(0 == wcscmp(parsed.value, L"value\\"));

  parsed = eini_parse("key='value\\\\\\'"); // 'value\\\'
  CU_ASSERT_EQUAL(parsed.type, EINI_ERROR);
  CU_ASSERT(0 == wcscmp(parsed.value, L"Non-terminated quote"));

  parsed = eini_parse("key=value ; comment");
  CU_ASSERT_EQUAL(parsed.type, EINI_VALUE);
  CU_ASSERT(0 == wcscmp(parsed.key, L"key"));
  CU_ASSERT(0 == wcscmp(parsed.value, L"value"));

  parsed = eini_parse("key='value' ; comment");
  CU_ASSERT_EQUAL(parsed.type, EINI_VALUE);
  CU_ASSERT(0 == wcscmp(parsed.key, L"key"));
  CU_ASSERT(0 == wcscmp(parsed.value, L"value"));

  parsed = eini_parse("key=\"value ; comment\"");
  CU_ASSERT_EQUAL(parsed.type, EINI_VALUE);
  CU_ASSERT(0 == wcscmp(parsed.key, L"key"));
  CU_ASSERT(0 == wcscmp(parsed.value, L"value ; comment"));

  parsed = eini_parse("key='value ; comment");
  CU_ASSERT_EQUAL(parsed.type, EINI_ERROR);
  CU_ASSERT(0 == wcscmp(parsed.value, L"Non-terminated quote"));

  parsed = eini_parse("blah");
  CU_ASSERT_EQUAL(parsed.type, EINI_ERROR);
  CU_ASSERT(0 == wcscmp(parsed.value, L"Unable to parse 'blah'"));

  eini_winddown();
}

// Tests for `eini()`

wchar_t *test_eini_output[256]; // `test_eini_handler()` and `test_eini_error()`
                                // store their results in here whenever they are
                                // called
unsigned test_eini_output_i;    // index for `test_eini_output`; identifies the
                                // location where the next result will be stored

// Test data
char *test_eini_data_1 = "; Config file without any errors\n"
                         "\n"
                         "[section1]\n"
                         "opt1=value #1\n"
                         "opt2=value #2\n"
                         "\n"
                         "[section2]\n"
                         "opt3=value #3";

char *test_eini_data_2 = "; Inclusion of a file that does not exist\n"
                         "\n"
                         "include /path/to/a/file/that/does/not/exist\n";

char *test_eini_data_3 = "; Configuration option without a section\n"
                         "\n"
                         "\n"
                         "opt1=value #1";

char *test_eini_data_4 = "; Syntax error\n"
                         "\n"
                         "yadayada bah poo";

// Handler function for `eini()`
void test_eini_handler(const wchar_t *section, const wchar_t *key,
                       const wchar_t *value, const char *path,
                       const unsigned line) {
  wchar_t *output = calloc(EINI_LONG, sizeof(wchar_t));

  swprintf(output, EINI_LONG, L"%s:%d -- %ls.%ls=%ls", path, line, section, key,
           value);
  test_eini_output[test_eini_output_i++] = output;
}

// Error function for `eini()`
void test_eini_error(const wchar_t *error, const char *path,
                     const unsigned line) {
  wchar_t *output = calloc(EINI_LONG, sizeof(wchar_t));

  swprintf(output, EINI_LONG, L"%s:%d -- %ls", path, line, error);
  test_eini_output[test_eini_output_i++] = output;
}

// Main test function
void test_eini() {
  char tpath[EINI_SHORT];      // path to a temporary config file
  FILE *tp;                    // file handler for `tpath`
  wchar_t expected[EINI_LONG]; // expected result

  strlcpy(tpath, "testsXXXXXX", EINI_SHORT);
  close(mkstemp(tpath));
  CU_ASSERT(0 != strcmp(tpath, "testsXXXXXX"));
  test_eini_output_i = 0;
  eini_init();

  eini(test_eini_handler, test_eini_error,
       "/path/to/a/file/that/does/not/exist");
  CU_ASSERT_EQUAL(test_eini_output_i, 1);
  CU_ASSERT(0 == wcscmp(test_eini_output[0],
                        L"/path/to/a/file/that/does/not/exist:0 -- Unable to "
                        L"open '/path/to/a/file/that/does/not/exist'"));

  tp = fopen(tpath, "w");
  CU_ASSERT_NOT_EQUAL(tp, NULL);
  fwrite(test_eini_data_1, sizeof(char), strlen(test_eini_data_1), tp);
  fclose(tp);
  eini(test_eini_handler, test_eini_error, tpath);
  CU_ASSERT_EQUAL(test_eini_output_i, 4);
  swprintf(expected, EINI_LONG, L"%s:4 -- section1.opt1=value #1", tpath);
  CU_ASSERT(0 == wcscmp(test_eini_output[1], expected))
  swprintf(expected, EINI_LONG, L"%s:5 -- section1.opt2=value #2", tpath);
  CU_ASSERT(0 == wcscmp(test_eini_output[2], expected))
  swprintf(expected, EINI_LONG, L"%s:8 -- section2.opt3=value #3", tpath);
  CU_ASSERT(0 == wcscmp(test_eini_output[3], expected))

  tp = fopen(tpath, "w");
  CU_ASSERT_NOT_EQUAL(tp, NULL);
  fwrite(test_eini_data_2, sizeof(char), strlen(test_eini_data_2), tp);
  fclose(tp);
  eini(test_eini_handler, test_eini_error, tpath);
  CU_ASSERT_EQUAL(test_eini_output_i, 5);
  swprintf(expected, EINI_LONG,
           L"%s:3 -- Unable to open '/path/to/a/file/that/does/not/exist'",
           tpath);
  CU_ASSERT(0 == wcscmp(test_eini_output[4], expected));

  tp = fopen(tpath, "w");
  CU_ASSERT_NOT_EQUAL(tp, NULL);
  fwrite(test_eini_data_3, sizeof(char), strlen(test_eini_data_3), tp);
  fclose(tp);
  eini(test_eini_handler, test_eini_error, tpath);
  CU_ASSERT_EQUAL(test_eini_output_i, 6);
  swprintf(expected, EINI_LONG, L"%s:4 -- Option 'opt1' does not have a section",
           tpath);
  CU_ASSERT(0 == wcscmp(test_eini_output[5], expected));

  tp = fopen(tpath, "w");
  CU_ASSERT_NOT_EQUAL(tp, NULL);
  fwrite(test_eini_data_4, sizeof(char), strlen(test_eini_data_4), tp);
  fclose(tp);
  eini(test_eini_handler, test_eini_error, tpath);
  CU_ASSERT_EQUAL(test_eini_output_i, 7);
  swprintf(expected, EINI_LONG, L"%s:3 -- Unable to parse 'yadayada bah poo'",
           tpath);
  CU_ASSERT(0 == wcscmp(test_eini_output[6], expected));

  eini_winddown();
  for (unsigned i = 0; i < test_eini_output_i; i++)
    free(test_eini_output[i]);
  unlink(tpath);
}

// Where we hope it works
int main(int argc, char **argv) {
  setlocale(LC_ALL, "");
  init_test_suite();

  // `add_test()` all your tests here
  add_test(eini_parse);
  add_test(eini);

  run_tests_and_exit();
}
