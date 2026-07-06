#!/bin/busybox sh
test "$0" = "/bin/shebangtest" && test "$1" = "ok" && echo SHEBANG_EXEC_OK
