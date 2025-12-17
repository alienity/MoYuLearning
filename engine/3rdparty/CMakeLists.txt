set(third_party_folder "ThirdParty")

set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

# Disable tests for all subprojects
set(BUILD_TESTING OFF)
set(SKIP_TESTS ON)
set(BUILD_EXAMPLES OFF)
set(BUILD_EXAMPLE OFF)

if(NOT TARGET imgui)
    include(imgui.cmake)
    set_target_properties(imgui PROPERTIES FOLDER ${third_party_folder}/imgui)
endif()

if(NOT TARGET ImGuizmo)
    include(ImGuizmo.cmake)
    set_target_properties(ImGuizmo PROPERTIES FOLDER ${third_party_folder}/ImGuizmo)
endif()

if(NOT TARGET ImGuiFileDialog)
    include(ImGuiFileDialog.cmake)
    set_target_properties(ImGuiFileDialog PROPERTIES FOLDER ${third_party_folder}/ImGuiFileDialog-Lib_Only)
endif()

if(NOT TARGET DirectX-Headers)
    add_subdirectory(DirectX-Headers)
    set_target_properties(DirectX-Headers PROPERTIES FOLDER ${third_party_folder}/DirectX-Headers)
    set_target_properties(DirectX-Guids PROPERTIES FOLDER ${third_party_folder}/DirectX-Guids)
endif()

if(NOT TARGET DirectXMath)
    add_subdirectory(DirectXMath-feb2024)
    set_target_properties(DirectXMath PROPERTIES FOLDER ${third_party_folder}/DirectXMath-feb2024)
endif()

if(NOT TARGET DirectXTK12)
    add_subdirectory(DirectXTK12-sept2023)
    set_target_properties(DirectXTK12 PROPERTIES FOLDER ${third_party_folder}/DirectXTK12-sept2023)
endif()

if(NOT TARGET glm)
    option(BUILD_STATIC_LIBS "" ON)
    option(BUILD_TESTING "" OFF)
    option(GLM_TEST_ENABLE "" OFF)
    add_subdirectory(glm-1.0.1)
    set_target_properties(glm PROPERTIES FOLDER ${third_party_folder}/glm-1.0.1)
endif()

if(NOT TARGET glfw)
    option(GLFW_BUILD_EXAMPLES "" OFF)
    option(GLFW_BUILD_TESTS "" OFF)
    option(GLFW_BUILD_DOCS "" OFF)
    option(GLFW_INSTALL "" OFF)
    add_subdirectory(glfw)
    set_target_properties(glfw PROPERTIES FOLDER ${third_party_folder}/glfw)
    set_target_properties(update_mappings PROPERTIES FOLDER ${third_party_folder}/glfw)
endif()

if(NOT TARGET spdlog)
	option(SPDLOG_WCHAR_SUPPORT "" ON)
    option(SPDLOG_BUILD_EXAMPLE "" OFF)
    option(SPDLOG_INSTALL "" OFF)
    add_subdirectory(spdlog)
    set_target_properties(spdlog PROPERTIES FOLDER ${third_party_folder}/spdlog)
endif()

if(NOT TARGET stb)
    include(stb.cmake)
    set_target_properties(stb PROPERTIES FOLDER ${third_party_folder}/stb)
endif()

if(NOT TARGET tinyexr)
    include(tinyexr.cmake)
    set_target_properties(tinyexr PROPERTIES FOLDER ${third_party_folder}/tinyexr-1.0.12)
    set_target_properties(miniz PROPERTIES FOLDER ${third_party_folder}/tinyexr-1.0.12/deps)
endif()

if(NOT TARGET tinyobjloader)
    add_subdirectory(tinyobjloader)
    set_target_properties(tinyobjloader PROPERTIES FOLDER ${third_party_folder}/tinyobjloader)
    set_target_properties(uninstall PROPERTIES FOLDER ${third_party_folder}/tinyobjloader)
endif()

if(NOT TARGET gvpp)
    include(gvpp.cmake)
	set_target_properties(gvpp PROPERTIES FOLDER ${third_party_folder}/gvpp)
endif()

# Add zlib library
set(SKIP_INSTALL_ALL ON)
set(ZLIB_BUILD_EXAMPLES OFF)
set(BUILD_SHARED_LIBS OFF)

if(NOT TARGET zlibstatic)
    add_subdirectory(zlib-1.3.1)
    set_target_properties(zlibstatic PROPERTIES FOLDER ${third_party_folder}/zlib-1.3.1)
endif()

# Ensure zlib is built before libpng and assimp
if(NOT TARGET zlibstatic)
    message(FATAL_ERROR "zlibstatic target not found. zlib must be built before libpng and assimp.")
endif()

# Set ZLIB variables globally to ensure all libraries use our zlibstatic target
set(ZLIB_FOUND TRUE)
set(ZLIB_LIBRARIES zlibstatic)
set(ZLIB_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/zlib-1.3.1 ${CMAKE_CURRENT_BINARY_DIR}/zlib-1.3.1)

if(NOT TARGET assimp)
    option(BUILD_SHARED_LIBS "" OFF)
    option(ASSIMP_BUILD_ZLIB "" OFF)
    option(ASSIMP_BUILD_TESTS "" OFF)
    option(ASSIMP_INJECT_DEBUG_POSTFIX "" OFF)
    # Set ZLIB variables to ensure assimp uses our zlibstatic target
    set(ZLIB_FOUND TRUE)
    set(ZLIB_LIBRARIES zlibstatic)
    set(ZLIB_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/zlib-1.3.1 ${CMAKE_CURRENT_BINARY_DIR}/zlib-1.3.1)
    add_subdirectory(assimp-5.2.4)
    set_target_properties(uninstall PROPERTIES FOLDER ${third_party_folder}/assimp-5.2.4)
    set_target_properties(assimp PROPERTIES FOLDER ${third_party_folder}/assimp-5.2.4)
    set_target_properties(assimp_cmd PROPERTIES FOLDER ${third_party_folder}/assimp-5.2.4)
    set_target_properties(UpdateAssimpLibsDebugSymbolsAndDLLs PROPERTIES FOLDER ${third_party_folder}/assimp-5.2.4)
endif()

if(NOT TARGET Jolt)
    option(TARGET_HELLO_WORLD "" OFF)
    option(TARGET_PERFORMANCE_TEST "" OFF)
    option(TARGET_SAMPLES "" OFF)
    option(TARGET_UNIT_TESTS "" OFF)
    option(TARGET_VIEWER "" OFF)

    if(ENABLE_PHYSICS_DEBUG_RENDERER)
        option(TARGET_TEST_FRAMEWORK "" ON)
    else()
        option(TARGET_TEST_FRAMEWORK "" OFF)
    endif()

    add_subdirectory(JoltPhysics/Build)

    if(ENABLE_PHYSICS_DEBUG_RENDERER)
        set_target_properties(Jolt TestFramework
            PROPERTIES 
            FOLDER ${third_party_folder}/JoltPhysics
            MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")
    else()
        set_target_properties(Jolt
            PROPERTIES 
            FOLDER ${third_party_folder}/JoltPhysics
            MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")
    endif()
endif()

if(NOT TARGET dxc)
    include(dxc.cmake)
    set_target_properties(dxc PROPERTIES FOLDER ${third_party_folder}/dxc)
endif()

if(NOT TARGET pix)
    include(pix.cmake)
    set_target_properties(pix PROPERTIES FOLDER ${third_party_folder}/pix)
endif()

if(NOT TARGET DirectStorage)
    include(DirectStorage.cmake)
    set_target_properties(DirectStorage PROPERTIES FOLDER ${third_party_folder}/DirectStorage)
endif()

if(NOT TARGET cityhash)
    include(cityhash.cmake)
    set_target_properties(cityhash PROPERTIES FOLDER ${third_party_folder}/cityhash)
endif()

if(NOT TARGET DirectXTex)
    add_subdirectory(DirectXTex)
    set_target_properties(DirectXTex PROPERTIES FOLDER ${third_party_folder}/DirectXTex)
endif()

if(NOT TARGET FMT)
    set(FMT_TEST OFF)
    add_subdirectory(fmt-10.1.1)
    set_target_properties(fmt PROPERTIES FOLDER ${third_party_folder}/fmt-10.1.1)
endif()

# Add abseil-cpp library
set(ABSL_BUILD_TESTING OFF)
set(ABSL_BUILD_TEST_HELPERS OFF)
set(BUILD_TESTING OFF)

if(NOT TARGET absl::base)
    add_subdirectory(abseil-cpp-20250814.1)
    set_target_properties(absl_base PROPERTIES FOLDER ${third_party_folder}/abseil-cpp-20250814.1)
endif()

if(NOT TARGET basisu)
    set(EXAMPLES OFF)
    add_subdirectory(basis_universal-before_xuastc basis_universal-before_xuastc)
    set_target_properties(basisu_encoder PROPERTIES FOLDER "${third_party_folder}/basisu_encoder")
    set_target_properties(basisu PROPERTIES FOLDER ${third_party_folder}/basis_universal-before_xuastc)
endif()

if(NOT TARGET eigen)
    add_subdirectory(eigen-master)
    set_target_properties(eigen PROPERTIES FOLDER ${third_party_folder}/eigen-master)
endif()

if(NOT TARGET embree)
    # Embree uses a different structure, need to import it differently
    # add_subdirectory(embree-4.3.3.x64.windows)
    # set_target_properties(embree PROPERTIES FOLDER ${third_party_folder}/embree-4.3.3.x64.windows)
endif()

if(NOT TARGET getopt)
    add_subdirectory(getopt)
    set_target_properties(getopt PROPERTIES FOLDER ${third_party_folder}/getopt)
endif()

# Add libpng library
set(PNG_TESTS OFF)
set(PNG_TOOLS OFF)
set(PNG_SHARED OFF)
set(PNG_STATIC ON)
set(ZLIB_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/zlib-1.3.1)
set(ZLIB_LIBRARY zlibstatic)
set(ZLIB_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/zlib-1.3.1 ${CMAKE_CURRENT_BINARY_DIR}/zlib-1.3.1)

if(NOT TARGET png)
    add_subdirectory(lpng1653)
    set_target_properties(png_static PROPERTIES FOLDER ${third_party_folder}/lpng1653)
    set_target_properties(png_genfiles PROPERTIES FOLDER ${third_party_folder}/lpng1653)
endif()

# Add tinyexr library
set(TINYEXR_BUILD_SAMPLE OFF CACHE BOOL "Build a sample" FORCE)
set(DISABLE_TINYEXR_SAMPLE ON CACHE BOOL "Disable tinyexr sample completely" FORCE)

if(NOT TARGET robin_hood)
    # robin_hood is header-only, no build required
endif()

if(NOT TARGET robin_map)
    add_subdirectory(tsl tsl)
    set_target_properties(robin_map PROPERTIES FOLDER ${third_party_folder}/tsl)
endif()

if(NOT TARGET nlohmann_json)
    # nlohmann json is header-only, no build required
endif()

# if(NOT TARGET WIL)
#     set(WIL_BUILD_TESTS OFF)
#     add_subdirectory(wil)
#     set_target_properties(WIL PROPERTIES FOLDER ${third_party_folder}/wil)
# endif()

# if(NOT TARGET directxtex)
#     include(directxtex.cmake)
#     set_target_properties(directxtex PROPERTIES FOLDER ${third_party_folder}/directxtex_desktop_win10.2023.6.14.1)
# endif()