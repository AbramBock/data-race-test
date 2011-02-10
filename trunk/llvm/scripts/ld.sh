#!/bin/bash
#
# A g++ wrapper that links the instrumented object files together with
# ThreadSanitizer RTL. It also puts together all the debug information
# generated by opt.

SCRIPT_ROOT=`dirname $0`

source "$SCRIPT_ROOT/common.sh"

SRC_EXE=""

ARGS=
# TODO(glider): x86-64 should be the default arch
#PLATFORM="x86-64"

SHARED=

until [ -z "$1" ]
do
  if [ `expr match "$1" "-o"` -gt 0 ]
  then
    if [ "$1" == "-o" ]
    then
      # "-o exename" -- 2 arguments
      shift
      SRC_EXE="$1"
    else
      # "-oexename" -- 1 argument
      SRC_EXE=${1:2}
    fi
  elif [ `expr match "$1" "-m64"` -gt 0 ]
  then
    PLATFORM="x86-64"
  elif [ `expr match "$1" "-m32"` -gt 0 ]
  then
    PLATFORM="x86"
  # TODO(glider): make tsan_rtl_debug_info not collectible by
  # --gc-sections.
  elif [ `expr match "$1" "-Wl,--gc-sections"` -gt 0 ]
  then
    # pass
    true
  elif [ `expr match "$1" "-shared"` -gt 0 ]
  then
    SHARED=1
  elif [ `expr match "$1" ".*\.[ao]"` -gt 0 ]
  then
    ARGS="$ARGS $1"
  else
    ARGS="$ARGS $1"
  fi
  shift
done

source "$SCRIPT_ROOT/link_config.sh"

INST_MODE=-offline
INST_MODE=-online
CXXFLAGS=

LOG=instrumentation.log

set_platform_dependent_vars
if [ "$SHARED" != "1" ]
then
  # Link with $TSAN_RTL
  $TSAN_LD $MARCH  $ARGS $LDFLAGS $TSAN_RTL -o $SRC_EXE || exit 1
else
  # The target is a .so library -- don't link it with $TSAN_RTL.
  $TSAN_LD $MARCH -shared  $ARGS -o $SRC_EXE || exit 1
fi