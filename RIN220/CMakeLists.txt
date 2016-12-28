# myApp/CMakeLists.txt
include_directories(headers)
file( GLOB_RECURSE MY_SOURCE_FILES "sources/*.cpp" "sources/*.c" )
file( GLOB_RECURSE MY_INC_FILES "headers/*.h" "headers/*.hpp" )
add_executable(RIN220 ${MY_SOURCE_FILES} ${MY_INC_FILES})
#add_executable(myApp main.cpp)
target_link_libraries(RIN220  gpstk)