# cmake/GitVersion.cmake
# Reads the VERSION file from the repository root for the authoritative version.
# Expected format: v1.0.0.0
#
# On success, sets in parent scope:
#   VERSION_FULL       — full version string e.g. "v1.0.0.0"
#   VERSION_CLEAN      — version without "v" prefix e.g. "1.0.0.0"
#   VERSION_MAJOR
#   VERSION_MINOR
#   VERSION_PATCH
#   VERSION_TWEAK
#
# Fails with a fatal error if:
#   - VERSION file does not exist
#   - VERSION file content does not match expected vX.X.X.X format

set(VERSION_FILE "${CMAKE_SOURCE_DIR}/VERSION")

# Ensure VERSION file exists
if(NOT EXISTS "${VERSION_FILE}")
    message(FATAL_ERROR
        "\n"
        "  error-dashboard: VERSION file not found at ${VERSION_FILE}.\n"
        "  This file is required and must contain a version in the format v1.0.0.0.\n"
    )
endif()

# Read and strip whitespace
file(READ "${VERSION_FILE}" VERSION_FULL)
string(STRIP "${VERSION_FULL}" VERSION_FULL)

# Validate format strictly: must be vMAJOR.MINOR.PATCH.TWEAK
if(NOT VERSION_FULL MATCHES "^v([0-9]+)\\.([0-9]+)\\.([0-9]+)\\.([0-9]+)$")
    message(FATAL_ERROR
        "\n"
        "  error-dashboard: VERSION file contains '${VERSION_FULL}' which does not match the required format.\n"
        "  Expected format: v1.0.0.0 (vMAJOR.MINOR.PATCH.TWEAK)\n"
    )
endif()

# Extract components
string(REGEX REPLACE "^v([0-9]+)\\.([0-9]+)\\.([0-9]+)\\.([0-9]+)$"
    "\\1" VERSION_MAJOR "${VERSION_FULL}")
string(REGEX REPLACE "^v([0-9]+)\\.([0-9]+)\\.([0-9]+)\\.([0-9]+)$"
    "\\2" VERSION_MINOR "${VERSION_FULL}")
string(REGEX REPLACE "^v([0-9]+)\\.([0-9]+)\\.([0-9]+)\\.([0-9]+)$"
    "\\3" VERSION_PATCH "${VERSION_FULL}")
string(REGEX REPLACE "^v([0-9]+)\\.([0-9]+)\\.([0-9]+)\\.([0-9]+)$"
    "\\4" VERSION_TWEAK "${VERSION_FULL}")

string(REPLACE "v" "" VERSION_CLEAN "${VERSION_FULL}")

# Propagate to parent scope
set(VERSION_FULL   ${VERSION_FULL}   PARENT_SCOPE)
set(VERSION_CLEAN  ${VERSION_CLEAN}  PARENT_SCOPE)
set(VERSION_MAJOR  ${VERSION_MAJOR}  PARENT_SCOPE)
set(VERSION_MINOR  ${VERSION_MINOR}  PARENT_SCOPE)
set(VERSION_PATCH  ${VERSION_PATCH}  PARENT_SCOPE)
set(VERSION_TWEAK  ${VERSION_TWEAK}  PARENT_SCOPE)

message(STATUS "error-dashboard version: ${VERSION_FULL}")