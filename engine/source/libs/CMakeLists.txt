add_subdirectory(${LIBS_DIR}/ibl ibl)

# Add abseil-cpp library
set(ABSL_BUILD_TESTING OFF)
set(ABSL_BUILD_TEST_HELPERS OFF)
set(BUILD_TESTING OFF)
# Note: abseil is actually a third-party library, should be moved to 3rdparty

# Disable tests for all subprojects
set(BUILD_TESTING OFF)
set(SKIP_TESTS ON)
set(BUILD_EXAMPLES OFF)
set(BUILD_EXAMPLE OFF)
set(BUILD_SHARED_LIBS OFF)

add_subdirectory(image image)
set_target_properties(image PROPERTIES FOLDER "Libs")

add_subdirectory(imageio imageio)
set_target_properties(imageio PROPERTIES FOLDER "Libs")

# Add utils library
set(UTILS_BUILD_TESTS OFF)
set(UTILS_BUILD_BENCHMARKS OFF)
add_subdirectory(utils utils)
set_target_properties(utils PROPERTIES FOLDER "Libs")

# Add math library
add_subdirectory(math math)
set_target_properties(math PROPERTIES FOLDER "Libs")

# Note: The following third-party libraries should be moved to 3rdparty directory:
# - basis_universal-before_xuastc
# - zlib-1.3.1
# - getopt
# - lpng1653
# - tinyexr-1.0.12
