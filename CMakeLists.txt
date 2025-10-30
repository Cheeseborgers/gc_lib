cmake_minimum_required(VERSION 3.20)

project(gc_lib VERSION 1.0.0 DESCRIPTION "General C utility library" LANGUAGES C)

# --- Options ---
option(GC_LIB_BUILD_SHARED "Build shared library instead of static" OFF)
option(BUILD_TESTS "Build test executables" ON)

# --- Sources ---
set(GC_LIB_SOURCES
        src/gc_logger.c
        src/gc_rb_tree.c
)

# --- Target setup ---
set(TARGET_NAME gc_lib)

if(GC_LIB_BUILD_SHARED)
    add_library(${TARGET_NAME} SHARED ${GC_LIB_SOURCES})
else()
    add_library(${TARGET_NAME} STATIC ${GC_LIB_SOURCES})
endif()

if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
    message(STATUS "Building gc_lib standalone")
else()
    message(STATUS "Building gc_lib as subproject")
endif()

if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

target_include_directories(${TARGET_NAME}
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

set_target_properties(${TARGET_NAME} PROPERTIES
        VERSION ${PROJECT_VERSION}
        SOVERSION 1
        OUTPUT_NAME ${TARGET_NAME}
)

# --- Headers ---
file(GLOB PUBLIC_HEADERS "include/*.h")
set_target_properties(${TARGET_NAME} PROPERTIES PUBLIC_HEADER "${PUBLIC_HEADERS}")

# --- Installation ---
include(GNUInstallDirs)

install(TARGETS ${TARGET_NAME}
        EXPORT GCLibTargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# --- Export target for package use ---
install(EXPORT GCLibTargets
        FILE GCLibTargets.cmake
        NAMESPACE GCLib::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/GCLib
)

# --- Package configuration ---
include(CMakePackageConfigHelpers)

# Generate the config version file (for version checking)
write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/GCLibConfigVersion.cmake"
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion
)

# Create the main config file that points to the export file
configure_package_config_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/GCLibConfig.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/GCLibConfig.cmake"
        INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/GCLib
)

install(FILES
        "${CMAKE_CURRENT_BINARY_DIR}/GCLibConfig.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/GCLibConfigVersion.cmake"
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/GCLib
)
