cmake_minimum_required(VERSION 3.12)
project(split_bill CXX)

# Adding userver dependency
find_package(userver COMPONENTS core postgresql QUIET)
if(NOT userver_FOUND)  # Fallback to subdirectory usage
    # Enable userver libraries that are needed in this project
    set(USERVER_FEATURE_POSTGRESQL ON CACHE BOOL "" FORCE)

    # Compatibility mode: some systems don't support these features
    set(USERVER_FEATURE_CRYPTOPP_BLAKE2 OFF CACHE BOOL "" FORCE)
    set(USERVER_FEATURE_GRPC_CHANNELZ OFF CACHE BOOL "" FORCE)
    set(USERVER_FEATURE_REDIS_HI_MALLOC ON CACHE BOOL "" FORCE)

    if (EXISTS third_party/userver)
        message(STATUS "Using userver framework from third_party/userver")
        add_subdirectory(third_party/userver)
    else()
        message(FATAL_ERROR "Either install the userver or provide a path to it")
    endif()
endif()

userver_setup_environment()


# Common sources
add_library(${PROJECT_NAME}_objs OBJECT
        src/models/product.hpp
        src/models/product.cpp
        src/models/user-product.hpp
        src/models/user-product.cpp
        src/models/session.hpp
        src/models/user.hpp
        src/models/room.hpp
        src/models/room.cpp
        src/models/detailed-room.cpp
        src/models/detailed-room.hpp
        src/handlers/v1/products/filters.hpp
        src/handlers/v1/products/filters.cpp
        src/handlers/lib/auth.hpp
        src/handlers/lib/auth.cpp
        src/handlers/v1/products/add-product/view.hpp
        src/handlers/v1/products/add-product/view.cpp
        src/handlers/v1/products/get-product/view.hpp
        src/handlers/v1/products/get-product/view.cpp
        src/handlers/v1/products/get-products/view.hpp
        src/handlers/v1/products/get-products/view.cpp
        src/handlers/v1/products/delete-product/view.hpp
        src/handlers/v1/products/delete-product/view.cpp
        src/handlers/v1/register/view.hpp
        src/handlers/v1/register/view.cpp
        src/handlers/v1/login/view.hpp
        src/handlers/v1/login/view.cpp


        src/handlers/v1/user-products/filters.hpp
        src/handlers/v1/user-products/filters.cpp
        src/handlers/v1/user-products/add-user-to-product/view.hpp
        src/handlers/v1/user-products/add-user-to-product/view.cpp
        src/handlers/v1/user-products/get-user-products/view.hpp
        src/handlers/v1/user-products/get-user-products/view.cpp
        src/handlers/v1/user-products/get-user-product/view.hpp
        src/handlers/v1/user-products/get-user-product/view.cpp
        src/handlers/v1/user-products/update-user-product/view.hpp
        src/handlers/v1/user-products/update-user-product/view.cpp
        src/handlers/v1/rooms/create-room/view.cpp
        src/handlers/v1/rooms/create-room/view.hpp
        src/handlers/v1/rooms/get-created-rooms/view.cpp
        src/handlers/v1/rooms/get-created-rooms/view.hpp
        src/handlers/v1/rooms/get-all-rooms/view.cpp
        src/handlers/v1/rooms/get-all-rooms/view.hpp
        src/handlers/v1/rooms/get-room/view.cpp
        src/handlers/v1/rooms/get-room/view.hpp
        src/handlers/v1/rooms/get-room-user-prices/view.cpp
        src/handlers/v1/rooms/get-room-user-prices/view.hpp
        src/handlers/v1/rooms/update-room/view.cpp
        src/handlers/v1/rooms/update-room/view.hpp
        src/handlers/v1/rooms/join-room/view.cpp
        src/handlers/v1/rooms/join-room/view.hpp
)
target_link_libraries(${PROJECT_NAME}_objs PUBLIC userver::postgresql)


# The Service
add_executable(${PROJECT_NAME} src/main.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE ${PROJECT_NAME}_objs)


## Unit Tests
#add_executable(${PROJECT_NAME}_unittest
#        unittests/basic.cpp
#        unittests/test_filters.cpp
#)
#target_link_libraries(${PROJECT_NAME}_unittest PRIVATE ${PROJECT_NAME}_objs userver::utest)
#add_google_tests(${PROJECT_NAME}_unittest)


# Benchmarks
add_executable(${PROJECT_NAME}_benchmark
)
target_link_libraries(${PROJECT_NAME}_benchmark PRIVATE ${PROJECT_NAME}_objs userver::ubench)
add_google_benchmark_tests(${PROJECT_NAME}_benchmark)

# Functional Tests
include(third_party/UserverTestsuite.cmake)

userver_testsuite_add_simple()

# Install
include(GNUInstallDirs)

if(DEFINED ENV{PREFIX})
    message(STATUS "Set install prefix: $ENV{PREFIX}")
    file(TO_CMAKE_PATH "$ENV{PREFIX}" PREFIX_PATH)
    set(CMAKE_INSTALL_PREFIX ${PREFIX_PATH})
endif()

file(GLOB CONFIGS_FILES ${CMAKE_CURRENT_SOURCE_DIR}/configs/*.yaml ${CMAKE_CURRENT_SOURCE_DIR}/configs/*.json)

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES ${CONFIGS_FILES} DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/${PROJECT_NAME})
