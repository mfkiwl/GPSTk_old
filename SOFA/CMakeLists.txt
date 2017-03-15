# SOFA/CMakeLists.txt
include_directories(headers)
file( GLOB_RECURSE MY_SOURCE_FILES "*.cpp" "*.c" )
file( GLOB_RECURSE MY_INC_FILES "*.h" "*.hpp" )
add_library(SOFA ${MY_SOURCE_FILES} ${MY_INC_FILES})
