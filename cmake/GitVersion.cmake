# cmake/GitVersion.cmake
# Reads the current Git tag and extracts a 4-part version (MAJOR.MINOR.PATCH.TWEAK).
# Expected tag format: v1.0.0.0
#
# On success, sets in parent scope:
#   GIT_TAG_FULL       — full tag string e.g. "v1.0.0.0"
#   GIT_TAG_CLEAN      — tag without "v" prefix e.g. "1.0.0.0"
#   VERSION_MAJOR
#   VERSION_MINOR
#   VERSION_PATCH
#   VERSION_TWEAK
#
# Fails with a fatal error if:
#   - Git is not found
#   - No tag is found on the current commit
#   - The tag does not match the expected vX.X.X.X format

find_package(Git REQUIRED)

# Require an exact tag on HEAD — no dirty suffix, no commit distance
execute_process(
    COMMAND ${GIT_EXECUTABLE} describe --tags --exact-match --match "v[0-9]*.[0-9]*.[0-9]*.[0-9]*"
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_TAG_FULL
    ERROR_VARIABLE  GIT_TAG_ERROR
    OUTPUT_STRIP_TRAILING_WHITESPACE
    RESULT_VARIABLE GIT_TAG_RESULT
)

if(NOT GIT_TAG_RESULT EQUAL 0)
    message(FATAL_ERROR
        "\n"
        "  error-dashboard: No valid Git version tag found on HEAD.\n"
        "  A tag in the format v1.0.0.0 is required to configure the build.\n"
        "\n"
        "  To tag the current commit:\n"
        "    git tag v1.0.0.0\n"
        "    git push origin v1.0.0.0\n"
        "\n"
        "  Git output: ${GIT_TAG_ERROR}"
    )
endif()

# Validate format strictly: must be vMAJOR.MINOR.PATCH.TWEAK
if(NOT GIT_TAG_FULL MATCHES "^v([0-9]+)\\.([0-9]+)\\.([0-9]+)\\.([0-9]+)$")
    message(FATAL_ERROR
        "\n"
        "  error-dashboard: Git tag '${GIT_TAG_FULL}' does not match the required format.\n"
        "  Expected format: v1.0.0.0 (vMAJOR.MINOR.PATCH.TWEAK)\n"
    )
endif()

# Extract components
string(REGEX REPLACE "^v([0-9]+)\\.([0-9]+)\\.([0-9]+)\\.([0-9]+)$"
    "\\1" VERSION_MAJOR "${GIT_TAG_FULL}")
string(REGEX REPLACE "^v([0-9]+)\\.([0-9]+)\\.([0-9]+)\\.([0-9]+)$"
    "\\2" VERSION_MINOR "${GIT_TAG_FULL}")
string(REGEX REPLACE "^v([0-9]+)\\.([0-9]+)\\.([0-9]+)\\.([0-9]+)$"
    "\\3" VERSION_PATCH "${GIT_TAG_FULL}")
string(REGEX REPLACE "^v([0-9]+)\\.([0-9]+)\\.([0-9]+)\\.([0-9]+)$"
    "\\4" VERSION_TWEAK "${GIT_TAG_FULL}")

string(REPLACE "v" "" GIT_TAG_CLEAN "${GIT_TAG_FULL}")

# Propagate to parent scope
set(GIT_TAG_FULL   ${GIT_TAG_FULL}   PARENT_SCOPE)
set(GIT_TAG_CLEAN  ${GIT_TAG_CLEAN}  PARENT_SCOPE)
set(VERSION_MAJOR  ${VERSION_MAJOR}  PARENT_SCOPE)
set(VERSION_MINOR  ${VERSION_MINOR}  PARENT_SCOPE)
set(VERSION_PATCH  ${VERSION_PATCH}  PARENT_SCOPE)
set(VERSION_TWEAK  ${VERSION_TWEAK}  PARENT_SCOPE)

message(STATUS "error-dashboard version: ${GIT_TAG_FULL}")
