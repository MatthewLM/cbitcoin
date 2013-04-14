cmake_minimum_required(VERSION 2.8)

project(cbitcoin)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

find_package(OpenSSL)
include_directories(${OPENSSL_INCLUDE_DIR})

find_package(Event)
include_directories(${EVENT_INCLUDE_DIR})

include_directories(include)

file(GLOB core_SOURCES src/*.c)
add_library(cbitcoin SHARED ${core_SOURCES})

file(GLOB crypto_SOURCE dependencies/crypto/*.c)
add_library(cbitcoin-crypto SHARED ${crypto_SOURCE})
target_link_libraries(cbitcoin-crypto ${OPENSSL_LIBRARIES})

file(GLOB random_SOURCE dependencies/random/*.c)
add_library(cbitcoin-random SHARED ${random_SOURCE})

file(GLOB network_SOURCE dependencies/sockets/*.c)
add_library(cbitcoin-network SHARED ${network_SOURCE})
target_link_libraries(cbitcoin-network ${EVENT_LIBRARIES})

file(GLOB storage_SOURCE dependencies/storage/*.c)
add_library(cbitcoin-storage SHARED ${storage_SOURCE})
