cmake_minimum_required (VERSION 3.28)
project(dsmr-parser LANGUAGES CXX)
include(FetchContent)

# Download Doctest test framework
file(DOWNLOAD
  https://github.com/doctest/doctest/releases/download/v2.4.11/doctest.h
  ${CMAKE_BINARY_DIR}/doctest/doctest.h
  EXPECTED_MD5 0b7fbd89a158063beecba78eb8400fad)

# Download re2c
file(DOWNLOAD
  https://github.com/PolarGoose/re2c-for-Windows/releases/download/3.1/re2c.zip
  ${CMAKE_BINARY_DIR}/re2c/re2c.zip
  EXPECTED_MD5 75762f5773ba96f2de679e1b2aae8086)
file(ARCHIVE_EXTRACT
  INPUT ${CMAKE_BINARY_DIR}/re2c/re2c.zip
  DESTINATION ${CMAKE_BINARY_DIR}/re2c
  PATTERNS "*re2c.exe")
file(TO_NATIVE_PATH ${CMAKE_BINARY_DIR}/DsmrParser/DsmrParser re2cOutputFolder)

# Configure re2c code generation
add_custom_target(re2c_generate_code
  COMMAND
    mkdir ${re2cOutputFolder} || echo "'${re2cOutputFolder}' directory already exists"
  COMMAND
    ${CMAKE_BINARY_DIR}/re2c/re2c.exe
      ${CMAKE_SOURCE_DIR}/src/DsmrParser/DsmrParser.re2c.h
      --output ${re2cOutputFolder}/DsmrParser.h
      --no-debug-info
      --no-generation-date
      --no-version
  COMMENT
    "re2c generating"
  DEPENDS
    ${CMAKE_SOURCE_DIR}/src/DsmrParser/DsmrParser.re2c.h)

# Configure test project
file(GLOB_RECURSE src_files CONFIGURE_DEPENDS "src/test/*.h" "src/test/*.cpp" "src/DsmrParser/*.h" "${CMAKE_BINARY_DIR}/DsmrParser/DsmrParser/*.h")
add_executable(test_executable ${src_files})
target_include_directories(test_executable PRIVATE ${CMAKE_BINARY_DIR}/doctest ${CMAKE_BINARY_DIR}/DsmrParser)
target_compile_options(test_executable PRIVATE /W4 /D_CRT_SECURE_NO_WARNINGS /permissive- /external:anglebrackets /external:W0)
target_compile_features(test_executable PUBLIC cxx_std_11)
add_dependencies(test_executable re2c_generate_code)

# Download and configure clang-format
file(DOWNLOAD
  https://github.com/muttleyxd/clang-tools-static-binaries/releases/download/master-f7f02c1d/clang-format-17_windows-amd64.exe
  ${CMAKE_BINARY_DIR}/clang-format.exe
  EXPECTED_MD5 459e1bec4b16540b098ac7bd893d5781)

add_custom_target(dsmrparser_clangformat
  COMMAND
    ${CMAKE_BINARY_DIR}/clang-format.exe -style=file -i ${src_files}
  WORKING_DIRECTORY
    ${CMAKE_SOURCE_DIR}
  COMMENT
    "Formatting source files with clang-format")
add_dependencies(dsmrparser_clangformat re2c_generate_code)
add_dependencies(test_executable dsmrparser_clangformat)
