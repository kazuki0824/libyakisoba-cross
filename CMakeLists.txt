cmake_minimum_required(VERSION 3.16)

project(libyakisoba
  VERSION 0.2
  LANGUAGES C
)

include(GNUInstallDirs)
include(CTest)
include(CMakePackageConfigHelpers)

option(BUILD_SHARED_LIBS "Build shared libraries" ON)
option(YAKISOBA_BUILD_TESTS "Build libyakisoba tests" ${BUILD_TESTING})
option(YAKISOBA_INSTALL "Install libyakisoba targets and files" ON)

add_library(yakisoba
  src/Crypto.c
  src/Decoder-ECM.c
  src/Decoder-EMM.c
  src/Keyset.c
)
add_library(yakisoba::yakisoba ALIAS yakisoba)

target_compile_features(yakisoba PUBLIC c_std_99)

target_include_directories(yakisoba
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/compat
)

target_compile_definitions(yakisoba
  PRIVATE
    "SYSCONF_DIR=\"${CMAKE_INSTALL_FULL_SYSCONFDIR}\""
)

if(BUILD_SHARED_LIBS)
  target_compile_definitions(yakisoba PUBLIC YAKISOBA_SHARED)
endif()

if(WIN32)
  target_link_libraries(yakisoba PRIVATE ws2_32)
endif()

if(MSVC)
  target_compile_options(yakisoba PRIVATE /W4)
else()
  target_compile_options(yakisoba PRIVATE -Wall -Wextra -Wpedantic -fno-strict-aliasing)
endif()

set_target_properties(yakisoba PROPERTIES
  OUTPUT_NAME yakisoba
  VERSION ${PROJECT_VERSION}
  SOVERSION 0
  C_VISIBILITY_PRESET hidden
)

if(YAKISOBA_BUILD_TESTS)
  add_executable(test-ecm src/test-ecm.c)
  target_link_libraries(test-ecm PRIVATE yakisoba::yakisoba)

  if(EMSCRIPTEN)
    target_link_options(test-ecm PRIVATE
      "SHELL:-sNODERAWFS=1"
      "SHELL:-sEXIT_RUNTIME=1"
    )
    find_program(NODE_EXECUTABLE node)
    if(NODE_EXECUTABLE)
      add_test(NAME test-ecm COMMAND ${NODE_EXECUTABLE} $<TARGET_FILE:test-ecm>)
    else()
      add_test(NAME test-ecm COMMAND $<TARGET_FILE:test-ecm>)
    endif()
  else()
    add_test(NAME test-ecm COMMAND test-ecm)
  endif()

  set_tests_properties(test-ecm PROPERTIES
    ENVIRONMENT "BCAS_KEYS_FILE=${CMAKE_CURRENT_SOURCE_DIR}/src/bcas_keys"
  )
endif()

if(YAKISOBA_INSTALL)
  install(TARGETS yakisoba
    EXPORT yakisobaTargets
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  )

  install(FILES src/yakisoba.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  )

  install(FILES src/bcas_keys
    DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}
  )

  install(EXPORT yakisobaTargets
    NAMESPACE yakisoba::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/yakisoba
  )

  configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/yakisobaConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/yakisobaConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/yakisoba
  )

  write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/yakisobaConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
  )

  install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/yakisobaConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/yakisobaConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/yakisoba
  )
endif()
