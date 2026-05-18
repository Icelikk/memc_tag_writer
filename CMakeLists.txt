cmake_minimum_required(VERSION 3.16)
project(MemcachedWriter LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(PLOG_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/third_party/plog/include)

find_library(LIBMEMCACHED_LIB memcached)
find_path(LIBMEMCACHED_INCLUDE libmemcached/memcached.h)

find_package(PostgreSQL REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(PQXX REQUIRED libpqxx)

# Добавьте эту строку, чтобы явно указать линковщику порядок
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -lpqxx -lpq")

add_executable(memcached_writer src/memc_writer.cpp)
target_include_directories(memcached_writer PRIVATE 
    ${LIBMEMCACHED_INCLUDE} 
    ${PLOG_INCLUDE_DIR}
)
target_link_libraries(memcached_writer 
    ${LIBMEMCACHED_LIB} 
    pthread 
    event
)

add_executable(memcached_to_pg src/pg_memc.cpp)
target_include_directories(memcached_to_pg PRIVATE 
    ${LIBMEMCACHED_INCLUDE} 
    ${PostgreSQL_INCLUDE_DIRS} 
    ${PQXX_INCLUDE_DIRS}
    ${PLOG_INCLUDE_DIR}
)
target_link_libraries(memcached_to_pg 
    ${LIBMEMCACHED_LIB} 
    ${PQXX_LIBRARIES}      
    ${PostgreSQL_LIBRARIES} 
    pthread 
    event
)