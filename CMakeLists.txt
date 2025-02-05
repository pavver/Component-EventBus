cmake_minimum_required(VERSION 3.16)

set(CMAKE_C_STANDARD 11)

if(DEFINED ENV{IDF_PATH})

    set(srcs
        "src/eventbus.c"
    )
    set(include_dirs "include")

    idf_component_register(SRCS ${srcs}
                           INCLUDE_DIRS ${include_dirs}
                           PRIV_REQUIRES ""
                           )

else()

    project(EventBusLibrary C)

    set(CMAKE_BUILD_TYPE Debug)
    set(CMAKE_C_FLAGS_DEBUG "-g")
    set(CMAKE_CXX_FLAGS_DEBUG "-g")

    include_directories(include)

    add_library(eventbus
        src/eventbus.c
    )

    add_executable(example examples/windows/example1.c)

    target_link_libraries(example eventbus)
endif()
