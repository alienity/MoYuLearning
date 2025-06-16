set(gvpp_SOURCE_DIR_ ${CMAKE_CURRENT_SOURCE_DIR}/gvpp/src)

file(GLOB gvpp_sources CONFIGURE_DEPENDS  
"${gvpp_SOURCE_DIR_}/gvpp.hpp" 
"${gvpp_SOURCE_DIR_}/gvpp.cpp")

add_library(gvpp STATIC ${gvpp_sources})

target_include_directories(gvpp PUBLIC ${gvpp_SOURCE_DIR_})