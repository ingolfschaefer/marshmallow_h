set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_VERSION 1)

if ("$ENV{BUILDROOT_HOST_DIR}" STREQUAL ""
    OR "$ENV{BUILDROOT_STAGING_DIR}" STREQUAL ""
    OR "$ENV{BUILDROOT_SYSTEM_PROCESSOR}" STREQUAL ""
    OR "$ENV{BUILDROOT_TARGET_DIR}" STREQUAL "")
	message(FATAL_ERROR "\n*** Invalid toolchain environment ***\n"
	                    "Please source the toolchain environment file.\n")
endif()
set(CMAKE_SYSTEM_PROCESSOR $ENV{BUILDROOT_SYSTEM_PROCESSOR})

set(BUILDROOT_HOST_DIR    $ENV{BUILDROOT_HOST_DIR}    CACHE INTERNAL "" FORCE)
set(BUILDROOT_STAGING_DIR $ENV{BUILDROOT_STAGING_DIR} CACHE INTERNAL "" FORCE)
set(BUILDROOT_TARGET_DIR  $ENV{BUILDROOT_TARGET_DIR}  CACHE INTERNAL "" FORCE)

set(CMAKE_FIND_ROOT_PATH ${BUILDROOT_STAGING_DIR}
                         ${BUILDROOT_TARGET_DIR} CACHE INTERNAL "")
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

set(BUILD_UNIT_TESTS OFF CACHE BOOL "")
