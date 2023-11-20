cmake_minimum_required(VERSION 3.13)

# Toolchain must be set before project() call.
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
        set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_LIST_DIR}/coralmicro/cmake/toolchain-arm-none-eabi-gcc.cmake)
endif()

project(coralmicro-app)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

include_directories(coralmicro)
add_subdirectory(coralmicro)

set(main_m7_SOURCES
        main_cm7.cc
        src/app/rpc/RpcServer.cpp
        src/app/ml/InferenceFactory.cpp
        src/app/thread/ThreadInference.cpp
        src/app/thread/ThreadReporter.cpp
        src/app/thread/QueueMainM7.cpp
        src/app/thread/QueueMain.cpp

        DATA
        coralmicro/models/tf2_ssd_mobilenet_v2_coco17_ptq_edgetpu.tflite
)


add_executable_m7(coralmicro-app
        ${main_m7_SOURCES}
        M4_EXECUTABLE
)


target_link_libraries(coralmicro-app
        libs_base-m7_freertos
        libs_rpc_http_server
        libs_rpc_utils
)

