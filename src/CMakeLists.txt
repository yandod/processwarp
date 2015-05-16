find_package(PkgConfig)

if(NOT ${CMAKE_SYSTEM_NAME} MATCHES "Emscripten")
  # LLVM (http://llvm.org/docs/CMake.html)
  find_package(LLVM REQUIRED CONFIG)
  message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
  message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")
  
  include_directories(${LLVM_INCLUDE_DIRS})
  llvm_map_components_to_libnames(llvm_libs support core irreader)
  list(APPEND extra_libs ${llvm_libs})
  add_definitions(${LLVM_DEFINITIONS})

  # GLFW
  pkg_search_module(GLFW REQUIRED glfw3)
  include_directories(${GLFW_INCLUDE_DIRS})
  string(REPLACE ";" " " GLFW_CFLAGS_STR "${GLFW_CFLAGS}")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${GLFW_CFLAGS_STR}")
  list(APPEND extra_libs ${GLFW_LIBRARIES})

  # libffi
  pkg_search_module(FFI libffi)
  if(FFI_FOUND)
    include_directories(${FFI_INCLUDE_DIRS})
    string(REPLACE ";" " " FFI_CFLAGS_STR "${FFI_CFLAGS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${FFI_CFLAGS_STR}")
    list(APPEND extra_libs ${FFI_LIBRARIES})
  else()
    find_library(FFI_LIBRARY NAMES ffi)
    set(extra_libs ${extra_libs} ${FFI_LIBRARY})
  endif()

    # gloox
    pkg_search_module(GLOOX REQUIRED gloox)
    string(REPLACE ";" " " GLOOX_CFLAGS_STR "${GLOOX_CFLAGS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${GLOOX_CFLAGS_STR}")
    list(APPEND extra_libs ${GLOOX_LIBRARIES})
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -W -Wall -Wno-unused-parameter")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0")

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    # MaxOSX
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")

elseif(${CMAKE_SYSTEM_NAME} MATCHES "Emscripten")
    # Emscripten
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-warn-absolute-paths")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --bind -s EXPORT_ALL=1")

else()
    # Linux or other UNIX like system
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
endif()

if(NOT ${CMAKE_SYSTEM_NAME} MATCHES "Emscripten")
    set(CORE_FILES
        convert.cpp
        data_store.cpp
        error.cpp
        func_store.cpp
        intrinsic_bit.cpp
        intrinsic_glfw3.cpp
        intrinsic_libc.cpp
        intrinsic_memory.cpp
        intrinsic_overflow.cpp
        intrinsic_posix.cpp
        intrinsic_va_arg.cpp
        intrinsic_warp.cpp
        llvm_asm_loader.cpp
        processwarp.cpp
        server.cpp
        stackinfo.cpp
        symbols.cpp
        type_based.cpp
        type_store.cpp
        util.cpp
        vmachine.cpp
        vmemory.cpp
        xmpp.cpp
    )

    add_executable(processwarp ${CORE_FILES})
    target_link_libraries(processwarp ${extra_libs})
endif()

if(NOT ${CMAKE_SYSTEM_NAME} MATCHES "Emscripten")
    set(LOADER_FILES
        convert.cpp
        data_store.cpp
        error.cpp
        func_store.cpp
        intrinsic_bit.cpp
        intrinsic_glfw3.cpp
        intrinsic_libc.cpp
        intrinsic_memory.cpp
        intrinsic_overflow.cpp
        intrinsic_posix.cpp
        intrinsic_va_arg.cpp
        intrinsic_warp.cpp
        llvm_asm_loader.cpp
        llvm_loader.cpp
        stackinfo.cpp
        symbols.cpp
        type_based.cpp
        type_store.cpp
        util.cpp
        vmachine.cpp
        vmemory.cpp
    )

    add_executable(loader ${LOADER_FILES})
    target_link_libraries(loader ${extra_libs})
endif()

if(${CMAKE_SYSTEM_NAME} MATCHES "Emscripten")
    set(WEBFRONT_FILES
        convert.cpp
        data_store.cpp
        error.cpp
        func_store.cpp
        intrinsic_bit.cpp
        intrinsic_glfw3.cpp
        intrinsic_libc.cpp
        intrinsic_memory.cpp
        intrinsic_overflow.cpp
        intrinsic_posix.cpp
        intrinsic_va_arg.cpp
        intrinsic_warp.cpp
        main.cpp
        stackinfo.cpp
        symbols.cpp
        type_based.cpp
        type_store.cpp
        util.cpp
        vmachine.cpp
        vmemory.cpp
    )

    add_executable(webfront ${WEBFRONT_FILES})
    target_link_libraries(webfront)
endif()