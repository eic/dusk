include(FetchContent)

FetchContent_Declare(
  Catch2
  URL https://github.com/catchorg/Catch2/archive/refs/tags/v3.6.0.tar.gz
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(Catch2)

add_executable(unit_tests
  test_history_parser.cpp
  test_svg_writer.cpp
)

target_include_directories(unit_tests PRIVATE
  ${PROJECT_SOURCE_DIR}/src
)
target_link_libraries(unit_tests PRIVATE Catch2::Catch2WithMain)
target_compile_features(unit_tests PRIVATE cxx_std_20)
if(DUSK_ENABLE_COVERAGE)
  if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(unit_tests PRIVATE -O0 -g --coverage)
    target_link_options(unit_tests PRIVATE --coverage)
  else()
    message(WARNING "Coverage requested, but compiler does not support --coverage")
  endif()
endif()

add_test(NAME unit_tests COMMAND unit_tests)
