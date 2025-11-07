cmake_minimum_required(VERSION 3.15)
project(OKX_MT5_Arbitrage VERSION 2.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Windows MSYS2/MINGW64 specific settings
if(WIN32)
    set(CMAKE_PREFIX_PATH "C:/msys64/mingw64")
    set(CURL_LIBRARY "C:/msys64/mingw64/lib/libcurl.dll.a")
    set(CURL_INCLUDE_DIR "C:/msys64/mingw64/include")
    set(OPENSSL_ROOT_DIR "C:/msys64/mingw64")
endif()

# Find required packages
find_package(CURL REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)

# Include directories
include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${CURL_INCLUDE_DIRS})
include_directories(${OPENSSL_INCLUDE_DIR})

# Print found libraries (for debugging)
message(STATUS "CURL_LIBRARIES: ${CURL_LIBRARIES}")
message(STATUS "OPENSSL_LIBRARIES: ${OPENSSL_LIBRARIES}")
message(STATUS "OPENSSL_INCLUDE_DIR: ${OPENSSL_INCLUDE_DIR}")

# Source files
set(SOURCES
    src/config.cpp
    src/http_client.cpp
    src/okx_signer.cpp
    src/okx_rest_api.cpp
)

# Headers
set(HEADERS
    include/config.h
    include/data_types.h
    include/http_client.h
    include/okx_signer.h
    include/okx_rest_api.h
    include/okx_websocket.h
)

# Create static library
add_library(okx_api STATIC ${SOURCES} ${HEADERS})
target_link_libraries(okx_api 
    ${CURL_LIBRARIES} 
    ${OPENSSL_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT}
)

# If using Windows, add ws2_32 and crypt32
if(WIN32)
    target_link_libraries(okx_api ws2_32 crypt32)
endif()

# Test executables
add_executable(test_config tests/test_config.cpp)
target_link_libraries(test_config okx_api)

add_executable(test_config_en tests/test_config_en.cpp)
target_link_libraries(test_config_en okx_api)

add_executable(test_api_validator tests/test_api_validator.cpp)
target_link_libraries(test_api_validator 
    okx_api 
    ${CURL_LIBRARIES} 
    ${OPENSSL_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT}
)

# Installation
install(TARGETS okx_api DESTINATION lib)
install(FILES ${HEADERS} DESTINATION include)
install(TARGETS test_config test_config_en test_api_validator DESTINATION bin)
