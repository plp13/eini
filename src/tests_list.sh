#!/usr/bin/env bash
# List all configured tests. This is a helper script used by meson.

FILE="tests.c"
MATCH="^[[:space:]]*add_test("
LTRIM="s/[[:space:]]*add_test(//g"
RTRIM="s/);//g"

exit_on_error() {
  if [ "X${1}" != "X0" ]
  then
    echo "Command failed"
    exit ${1}
  fi
}

cd "$( dirname "${BASH_SOURCE[0]}" )"
exit_on_error $?

grep "${MATCH}" "${FILE}" | sed "${LTRIM}" | sed "${RTRIM}"
exit_on_error $?

exit 0

