cmake_minimum_required(VERSION 3.16)
project(dusk VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(DUSK_BUILD_APPS "Build dusk and duskcut executables" ON)
option(DUSK_ENABLE_TESTS "Build unit tests" OFF)
option(DUSK_ENABLE_COVERAGE "Build with gcov/lcov coverage instrumentation" OFF)

if(DUSK_BUILD_APPS)
  # Find OCCT — hint at the eic-shell installation
  find_package(OpenCASCADE REQUIRED
    HINTS
      ENV OpenCASCADE_DIR
      /opt/software/linux-x86_64_v2/opencascade-7.9.1-tnvuxyqignnf2xgka4kufuj2j26va4tu/lib/cmake/opencascade
      /usr/lib/x86_64-linux-gnu/cmake/opencascade
      /opt/local/lib/cmake/opencascade
      /usr/lib/cmake/opencascade
  )

  message(STATUS "OpenCASCADE version: ${OpenCASCADE_VERSION}")
  message(STATUS "OpenCASCADE include: ${OpenCASCADE_INCLUDE_DIR}")

  # Common OCCT libraries needed by both targets
  set(OCC_LIBS
    TKernel
    TKMath
    TKBRep
    TKG3d
    TKGeomBase
    TKGeomAlgo
    TKTopAlgo
    TKMesh
    TKHLR
    TKXSBase
    TKDESTEP
    TKXCAF
    TKCDF
    TKVCAF
    TKCAFWin
    TKBool
    TKBO
    TKPrim
    TKShHealing
  )

  # Filter to only the libs that actually exist
  set(OCC_LINK_LIBS)
  foreach(lib ${OCC_LIBS})
    if(TARGET ${lib})
      list(APPEND OCC_LINK_LIBS ${lib})
    endif()
  endforeach()

  # dusk executable
  add_executable(dusk src/dusk.cc)
  target_include_directories(dusk PRIVATE
    ${OpenCASCADE_INCLUDE_DIR}
    src
  )
  target_link_libraries(dusk PRIVATE ${OCC_LINK_LIBS})
  target_compile_definitions(dusk PRIVATE ${OpenCASCADE_DEFINITIONS})

  # duskcut executable
  add_executable(duskcut src/duskcut.cc)
  target_include_directories(duskcut PRIVATE
    ${OpenCASCADE_INCLUDE_DIR}
    src
  )
  target_link_libraries(duskcut PRIVATE ${OCC_LINK_LIBS})
  target_compile_definitions(duskcut PRIVATE ${OpenCASCADE_DEFINITIONS})

  install(TARGETS dusk duskcut DESTINATION bin)
  install(PROGRAMS scripts/dusk_tweak DESTINATION bin)
endif()

if(DUSK_ENABLE_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()
