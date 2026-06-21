# Runs at build time (via cmake -P). Writes ${OUTPUT} with the current git
# short SHA so the app can display which commit it was built from. Only
# rewrites the file when the value changes, to avoid needless recompiles.
#
# Expects -DSRC=<source dir> and -DOUTPUT=<header path>.

execute_process(
    COMMAND git -C "${SRC}" rev-parse --short HEAD
    OUTPUT_VARIABLE GIT_SHA
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
    RESULT_VARIABLE GIT_RESULT)

if(NOT GIT_RESULT EQUAL 0 OR GIT_SHA STREQUAL "")
    set(GIT_SHA "dev")
endif()

# Mark the build dirty if there are uncommitted changes.
execute_process(
    COMMAND git -C "${SRC}" diff --quiet
    RESULT_VARIABLE GIT_DIRTY)
if(GIT_DIRTY EQUAL 1)
    set(GIT_SHA "${GIT_SHA}+")
endif()

set(CONTENT "#pragma once\n#define DEEPDAW_BUILD_ID \"${GIT_SHA}\"\n")

set(OLD "")
if(EXISTS "${OUTPUT}")
    file(READ "${OUTPUT}" OLD)
endif()

if(NOT OLD STREQUAL CONTENT)
    file(WRITE "${OUTPUT}" "${CONTENT}")
endif()
