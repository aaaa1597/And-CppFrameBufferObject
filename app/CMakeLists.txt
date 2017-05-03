cmake_minimum_required(VERSION 3.4.1)

include_directories(src/main/cpp/libpng)

add_library(testlibpng SHARED IMPORTED)
set_target_properties(testlibpng PROPERTIES IMPORTED_LOCATION ../../../../jniLibs/${ANDROID_ABI}/libtestlibpng.so)

add_library( testlib SHARED
                     src/main/cpp/CppSurfaceView.cpp
                     src/main/cpp/UtilPng.cpp
                     src/main/cpp/UtilsMatrix.cpp
                     )

find_library( log-lib log )

target_link_libraries( testlib ${log-lib} android EGL GLESv2 m testlibpng )
