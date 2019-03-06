cmake_minimum_required(VERSION 2.8)

#在项目中显示所有源码
file(GLOB files_current ../open_collector/*.cpp ../open_collector/*.h)

#g++编译参数
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -W -pthread -m64 -std=c++11")
#忽略特定警告
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-missing-field-initializers")

#第三方库库文件目录
link_directories("D:/boost_1_67_0/lib/mingw-w64/x64")
#第三方库头文件目录
include_directories("D:/boost_1_67_0" "../")

ADD_DEFINITIONS(-DBOOST_COROUTINES_NO_DEPRECATION_WARNING)


#输出项目名称
project(test_open_collector)

#编译项目所需源码
set(src_app main.cpp ${files_current})

add_executable(${PROJECT_NAME} ${src_app})
#链接第三方库
target_link_libraries (${PROJECT_NAME} ws2_32 wsock32 boost_system-mgw73-mt-s-x64-1_67 boost_thread-mgw73-mt-s-x64-1_67
    boost_coroutine-mgw73-mt-s-x64-1_67 boost_context-mgw73-mt-s-x64-1_67)
