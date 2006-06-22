#! /bin/sh

if test $# -ne 1
then
    echo "Usage:"
    echo "$0 test-executable"
    exit 1
fi

if $1 | diff -u "${srcdir}/$1.expected" - > "$1.output" 2>&1
then
  :
else
  ec=$?
  cat "$1.output"
  exit $ec
fi
