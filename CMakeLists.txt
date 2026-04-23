cmake_minimum_required(VERSION 3.10)
project(MemcachedWriter)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(PLOG_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/third_party/plog/include)

find_library(LIBMEMCACHED_LIB memcached)
find_path(LIBMEMCACHED_INCLUDE libmemcached/memcached.h)

find_package(PostgreSQL REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(PQXX REQUIRED libpqxx)

add_executable(memcached_writer memc_writer.cpp)
target_include_directories(memcached_writer PRIVATE ${LIBMEMCACHED_INCLUDE} ${PLOG_INCLUDE_DIR})
target_link_libraries(memcached_writer ${LIBMEMCACHED_LIB} pthread event)

add_executable(memcached_to_pg pg_memc.cpp)
target_include_directories(memcached_to_pg PRIVATE ${LIBMEMCACHED_INCLUDE} ${PostgreSQL_INCLUDE_DIRS} ${PQXX_INCLUDE_DIRS})
target_link_libraries(memcached_to_pg ${LIBMEMCACHED_LIB} ${PostgreSQL_LIBRARIES} ${PQXX_LIBRARIES} pthread event)