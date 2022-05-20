cmake_minimum_required(VERSION 3.14...3.22)

project(OrgBulkInvite LANGUAGES CXX)

# ---- Import tools ----

include(../cmake/tools.cmake)

# ---- Dependencies ----

include(../cmake/CPM.cmake)

CPMAddPackage(
  GITHUB_REPOSITORY jarro2783/cxxopts
  VERSION 3.0.0
  OPTIONS "CXXOPTS_BUILD_EXAMPLES NO" "CXXOPTS_BUILD_TESTS NO" "CXXOPTS_ENABLE_INSTALL YES"
)

CPMAddPackage(
  NAME fmt
  GIT_TAG 8.1.1
  GITHUB_REPOSITORY fmtlib/fmt
  OPTIONS "FMT_INSTALL YES" # create an installable target
)

CPMAddPackage(
  NAME curlpp
  GIT_TAG "v0.8.1"
  GITHUB_REPOSITORY jpbarrette/curlpp
)

if(curlpp_ADDED)
  target_include_directories(curlpp INTERFACE "${curlpp_SOURCE_DIR}/include")
endif()

CPMAddPackage(
  NAME nlohmann_json
  VERSION 3.10.5
  # the git repo is incredibly large, so we download the archived include directory
  URL https://github.com/nlohmann/json/releases/download/v3.10.5/include.zip
  URL_HASH SHA256=b94997df68856753b72f0d7a3703b7d484d4745c567f3584ef97c96c25a5798e
)

if (nlohmann_json_ADDED)
  add_library(nlohmann_json INTERFACE IMPORTED)
  target_include_directories(nlohmann_json INTERFACE ${nlohmann_json_SOURCE_DIR}/include)
endif()

CPMAddPackage(
  NAME cpp_dotenv
  GIT_TAG "v1.0.0-alpha"
  GITHUB_REPOSITORY adeharo9/cpp-dotenv
)

# ---- Create standalone executable ----

file(GLOB sources CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp)

add_executable(${PROJECT_NAME} ${sources})

set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 17 OUTPUT_NAME "BulkInvite")

target_link_libraries(${PROJECT_NAME} cxxopts fmt::fmt curlpp nlohmann_json cpp_dotenv)

macro(print_all_variables)
    message(STATUS "print_all_variables------------------------------------------{")
    get_cmake_property(_variableNames VARIABLES)
    foreach (_variableName ${_variableNames})
        message(STATUS "${_variableName}=${${_variableName}}")
    endforeach()
    message(STATUS "print_all_variables------------------------------------------}")
endmacro()

# print_all_variables()
