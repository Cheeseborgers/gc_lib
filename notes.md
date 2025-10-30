Project: gc\_lib Library Notes (Comprehensive)

* * *

1️⃣ Project Overview

**Library Name:** gc\_lib  
**Language:** C  
**Purpose:** A small, reusable "std-like" utility library for memory, string, and general helper functions.

* * *

2️⃣ Directory Layout

    gc_lib/
    ├── CMakeLists.txt         # Main build script
    ├── include/
    │   └── gc_lib.h           # Main public header
    ├── src/
    │   ├── library.c          # Main implementation
    │   ├── mem.c              # Optional memory utilities
    │   └── str.c              # Optional string utilities
    ├── cmake/
    │   └── GCLibConfig.cmake.in
    └── tests/
        ├── CMakeLists.txt
        └── test_hello.c


* * *

3️⃣ Main Components

**Headers:**

*   `gc_lib.h`: Main public API header (users include this).

*   Optional modular headers (`gc_mem.h`, `gc_str.h`) for subsystem-specific functions.


**Source Files:**

*   `.c` files implement the functions declared in headers.


* * *

4️⃣ CMake Options

**Static / Shared toggle:**

    cmake -B build -DGC_LIB_BUILD_SHARED=ON   # build shared library
    cmake -B build -DGC_LIB_BUILD_SHARED=OFF  # build static library (default)


**Tests / Demos:**

    option(BUILD_TESTS "Build test executables" ON)
    option(BUILD_DEMOS "Build demo executables" OFF)


**Example:**

    cmake -B build -DBUILD_TESTS=OFF -DBUILD_DEMOS=ON
    cmake --build build


* * *

5️⃣ Building the Library

    cmake -B build -DCMAKE_BUILD_TYPE=Release
    cmake --build build


*   Output: `libgc_lib.a` (static) or `libgc_lib.so` / `gc_lib.dll` (shared)


* * *

6️⃣ Installing the Library

    cmake --install build --prefix /usr/local


*   Installs library files, headers, and CMake package files.

*   Paths follow `GNUInstallDirs` conventions:

    *   `lib` → library binaries

    *   `include` → headers

    *   `lib/cmake/GCLib` → package config files


* * *

7️⃣ Using gc\_lib in Other Projects

**Option 1: Installed Library**

    find_package(GCLib REQUIRED)
    target_link_libraries(MyApp PRIVATE GCLib::gc_lib)


**Option 2: Subdirectory (vendored / monorepo)**

    add_subdirectory(path/to/gc_lib)
    target_link_libraries(MyApp PRIVATE gc_lib)


* * *

8️⃣ Header / Source Relationships

*   `gc_lib.h`: main public header

*   Other headers: modular components, optionally included

*   `.c` files implement functions


**Example:**

    // gc_lib.h
    #include "gc_mem.h"
    #include "gc_str.h"
    void gc_hello(void);
    
    // library.c
    #include "gc_lib.h"
    #include <stdio.h>
    void gc_hello(void) { printf("Hello from gc_lib!\n"); gc_mem_init(); }


* * *

9️⃣ Optional Test / Demo Setup

**tests/CMakeLists.txt:**

    add_executable(test_hello test_hello.c)
    target_link_libraries(test_hello PRIVATE gc_lib)
    add_test(NAME test_hello COMMAND test_hello)


**tests/test\_hello.c:**

    #include "gc_lib.h"
    int main(void) { gc_hello(); return 0; }


*   Can run manually or via `ctest`.

*   Built only if `BUILD_TESTS=ON`.


* * *

🔟 Best Practices / Notes

*   Default to static library for internal projects; shared optional.

*   Check `CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR` to detect standalone/subproject build.

*   Prefix functions with `gc_` to avoid symbol collisions.

*   Use `find_package(GCLib REQUIRED)` for downstream projects.

*   Add new headers to `include/` and sources to `src/`, update `PUBLIC_HEADERS` accordingly.

*   Functions declared in headers become available to downstream code once linked.


* * *

1️⃣1️⃣ CMakeLists.txt Overview

    cmake_minimum_required(VERSION 3.20)
    project(gc_lib VERSION 1.0.0 DESCRIPTION "General C utility library" LANGUAGES C)
    
    # Options
    option(GC_LIB_BUILD_SHARED "Build shared library instead of static" OFF)
    option(BUILD_TESTS "Build test executables" ON)
    
    # Sources
    set(GC_LIB_SOURCES src/library.c)
    
    # Library target setup
    set(TARGET_NAME gc_lib)
    if(GC_LIB_BUILD_SHARED)
        add_library(${TARGET_NAME} SHARED ${GC_LIB_SOURCES})
    else()
        add_library(${TARGET_NAME} STATIC ${GC_LIB_SOURCES})
    endif()
    
    # Subproject detection
    if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
        message(STATUS "Building gc_lib standalone")
    else()
        message(STATUS "Building gc_lib as subproject")
    endif()
    
    # Optional tests
    if(BUILD_TESTS)
        enable_testing()
        add_subdirectory(tests)
    endif()
    
    # Include directories
    target_include_directories(${TARGET_NAME}
        PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE:include>
    )
    
    # Target properties
    set_target_properties(${TARGET_NAME} PROPERTIES
        VERSION ${PROJECT_VERSION}
        SOVERSION 1
        OUTPUT_NAME ${TARGET_NAME}
    )
    
    # Public headers
    file(GLOB PUBLIC_HEADERS "include/*.h")
    set_target_properties(${TARGET_NAME} PROPERTIES PUBLIC_HEADER "${PUBLIC_HEADERS}")
    
    # Installation
    include(GNUInstallDirs)
    install(TARGETS ${TARGET_NAME}
        EXPORT GCLibTargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )
    
    install(EXPORT GCLibTargets
        FILE GCLibTargets.cmake
        NAMESPACE GCLib::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/GCLib
    )
    
    # Package config
    include(CMakePackageConfigHelpers)
    write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/GCLibConfigVersion.cmake"
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion
    )
    
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