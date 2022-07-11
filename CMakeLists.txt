PROJECT(robots)
CMAKE_MINIMUM_REQUIRED(VERSION 3.0)

SET(CMAKE_CXX_STANDARD 14)
SET(CMAKE_POSITION_INDEPENDENT_CODE ON)

SET(VERSION "0.0.0")

STRING(REGEX MATCHALL "([0-9]+)" VERSION_DIGITS "${VERSION}")

LIST(GET VERSION_DIGITS 0 CPACK_PACKAGE_VERSION_MAJOR)
LIST(GET VERSION_DIGITS 1 CPACK_PACKAGE_VERSION_MINOR)
LIST(GET VERSION_DIGITS 2 CPACK_PACKAGE_VERSION_PATCH)

SET(CPACK_PACKAGE_NAME "robots")
SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Google's robots.txt parser and matcher C++ library")
SET(CPACK_PACKAGE_VENDOR "Google Inc.")
SET(CPACK_PACAKGE_DESCRIPTION_FILE "${CMAKE_SOURCE_DIR}/README.md")
SET(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")

SET(CPACK_PACKAGE_INSTALL_DIRECTORY "${CPACK_PACKAGE_DESCRIPTION_SUMMARY} ${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}")
SET(CPACK_SOURCE_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}")

SET(base_with_ver "robots-[0-9]+\\\\.[0-9]+\\\\.[0-9]+")
SET(CPACK_SOURCE_IGNORE_FILES
    "/_CPack_Packages/"
    "/CMakeFiles/"
    "/.deps/"
    "^${base_with_ver}(-Source|-Linux)?/"
    "${base_with_ver}.tar\\\\.(gz|bz2|Z|lzma|xz)$"
    "\\\\.o$"
    "~$"
    "/\\\\.svn/"
    "/CMakeCache\\\\.txt$"
    "/CTestTestfile\\\\.cmake$"
    "/cmake_install\\\\.cmake$"
    "/CPackConfig\\\\.cmake$"
    "/CPackSourceConfig\\\\.cmake$"
    "/tags$"
    "^config\\\\.h$"
    "/install_manifest\\\\.txt$"
    "/Testing/"
    "ids-whitelist\\\\.txt"
    "/_Inline/"
    "/(B|build|BUILD)/"
    "/autom4te.cache/"
)

############ build options ##############

OPTION(ROBOTS_BUILD_STATIC "If ON, robots will build also the static library" ON)
OPTION(ROBOTS_BUILD_TESTS "If ON, robots will build test targets" OFF)
OPTION(ROBOTS_INSTALL "If ON, enable the installation of the targets" ON)

############ helper libs ############

INCLUDE(CPack)
INCLUDE(CheckCCompilerFlag)
INCLUDE(ExternalProject)

############ dependencies ##############

CONFIGURE_FILE(CMakeLists.txt.in libs/CMakeLists.txt)
EXECUTE_PROCESS(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" . RESULT_VARIABLE result WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/libs)

IF(result)
    MESSAGE(FATAL_ERROR "Failed to download dependencies: ${result}")
ENDIF()

EXECUTE_PROCESS(COMMAND ${CMAKE_COMMAND} --build . RESULT_VARIABLE result WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/libs)

IF(result)
    MESSAGE(FATAL_ERROR "Failed to download dependencies: ${result}")
ENDIF()

# abseil-cpp
IF(MSVC)
    # /wd4005  macro-redefinition
    # /wd4068  unknown pragma
    # /wd4244  conversion from 'type1' to 'type2'
    # /wd4267  conversion from 'size_t' to 'type2'
    # /wd4800  force value to bool 'true' or 'false' (performance warning)
    ADD_COMPILE_OPTIONS(/wd4005 /wd4068 /wd4244 /wd4267 /wd4800)
    ADD_DEFINITIONS(/DNOMINMAX /DWIN32_LEAN_AND_MEAN=1 /D_CRT_SECURE_NO_WARNINGS)
ENDIF(MSVC)

ADD_SUBDIRECTORY(${CMAKE_CURRENT_BINARY_DIR}/libs/abseil-cpp-src
                 ${CMAKE_CURRENT_BINARY_DIR}/libs/abseil-cpp-build
                 EXCLUDE_FROM_ALL)

IF(ROBOTS_BUILD_TESTS)
    INCLUDE(CTest)

    # googletest
    ADD_SUBDIRECTORY(${CMAKE_CURRENT_BINARY_DIR}/libs/gtest-src
                     ${CMAKE_CURRENT_BINARY_DIR}/libs/gtest-build
                     EXCLUDE_FROM_ALL)

    SET(INSTALL_GTEST 0)
    SET(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

    IF(CMAKE_VERSION VERSION_LESS 2.8.11)
        INCLUDE_DIRECTORIES(${gtest_SOURCE_DIR}/include)
    ENDIF()
ENDIF(ROBOTS_BUILD_TESTS)

########### compiler flags ##############


SET(COMPILER_FLAGS_TO_CHECK
    "-Wall" "-Werror=implicit-function-declaration"
)

IF(CPU_ARCH)
    LIST(APPEND COMPILER_FLAGS_TO_CHECK "-march=${CPU_ARCH}")
ENDIF(CPU_ARCH)

########### project files ###############

INCLUDE_DIRECTORIES(.)

######### targets ###########

SET(LIBROBOTS_LIBS)

SET(robots_SRCS ./robots.cc)
SET(robots_LIBS absl::base absl::strings)

ADD_LIBRARY(robots SHARED ${robots_SRCS})
TARGET_LINK_LIBRARIES(robots ${robots_LIBS})
LIST(APPEND LIBROBOTS_LIBS "robots")

IF(ROBOTS_BUILD_STATIC)
    ADD_LIBRARY(robots-static STATIC ${robots_SRCS})
    TARGET_LINK_LIBRARIES(robots-static ${robots_LIBS})

    LIST(APPEND LIBROBOTS_LIBS "robots-static")

    SET_TARGET_PROPERTIES(robots-static PROPERTIES OUTPUT_NAME "robots")

    SET_TARGET_PROPERTIES(${LIBROBOTS_LIBS} PROPERTIES CLEAN_DIRECT_OUTPUT 1)
ENDIF(ROBOTS_BUILD_STATIC)

IF(WIN_32)
    SET_TARGET_PROPERTIES(robots PROPERTIES DEFINE_SYMBOL DLL_EXPORT)
ENDIF(WIN_32)

ADD_EXECUTABLE(robots-main ./robots_main.cc)
TARGET_LINK_LIBRARIES(robots-main ${LIBROBOTS_LIBS})
SET_TARGET_PROPERTIES(robots-main PROPERTIES OUTPUT_NAME "robots")

############ installation ############

IF(ROBOTS_INSTALL)
    INSTALL(TARGETS ${LIBROBOTS_LIBS}
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
    )

    INSTALL(FILES ${CMAKE_SOURCE_DIR}/robots.h DESTINATION include)

    INSTALL(TARGETS robots-main DESTINATION bin)
ENDIF(ROBOTS_INSTALL)

############ tests ##############

IF(ROBOTS_BUILD_TESTS)
    ENABLE_TESTING()

    ADD_EXECUTABLE(robots-test ./robots_test.cc)
    TARGET_LINK_LIBRARIES(robots-test ${LIBROBOTS_LIBS} gtest_main)
    ADD_TEST(NAME robots-test COMMAND robots-test)
ENDIF(ROBOTS_BUILD_TESTS)

