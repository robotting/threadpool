cmake_minimum_required(VERSION 4.3)
project(ThreadPool VERSION 1.0.0 LANGUAGES CXX)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 设置构建类型
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING 
        "Build type: Debug, Release, RelWithDebInfo, MinSizeRel" FORCE)
endif()

# 输出路径
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# 编译选项
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")

elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    add_compile_options(/utf-8)

endif()

# 查找线程库
find_package(Threads REQUIRED)

# 创建线程池库
add_library(ThreadPool SHARED)
target_sources(ThreadPool PRIVATE src/thread_pool.cpp)
target_include_directories(ThreadPool 
    PUBLIC 
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)
target_link_libraries(ThreadPool PRIVATE Threads::Threads)

# 设置库属性
set_target_properties(ThreadPool PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
    OUTPUT_NAME "threadpool"
)

# 安装配置
include(GNUInstallDirs)
install(TARGETS ThreadPool
    EXPORT ThreadPoolTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/threadpool
)

install(EXPORT ThreadPoolTargets
    FILE ThreadPoolTargets.cmake
    NAMESPACE ThreadPool::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ThreadPool
)

# 添加子目录
#add_subdirectory(examples)
#add_subdirectory(tests)

# 启用测试
#enable_testing()