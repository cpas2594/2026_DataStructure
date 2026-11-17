cmake_minimum_required(VERSION 3.16)
project(SeoulEmergencySearch LANGUAGES CXX)

# 현재는 아키텍처만 준비된 상태입니다. 기능 코드는 추후 구현합니다.
add_executable(emergency_search
    src/main.cpp
    src/data/CsvLoader.cpp
    src/data/HospitalStore.cpp
    src/search/SearchService.cpp
    src/search/SearchEngine.cpp
    src/search/HospitalFilter.cpp
    src/ui/ConsoleUI.cpp
)

# C++17을 사용하며 컴파일러 전용 언어 확장은 사용하지 않습니다.
set_target_properties(emergency_search PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

# #include "data/CsvLoader.h"처럼 src 기준으로 헤더를 찾습니다.
target_include_directories(emergency_search PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

# 컴파일러는 사용자의 환경에서 선택합니다. 설치 경로를 고정하지 않습니다.
if(MSVC)
    target_compile_options(emergency_search PRIVATE /W4 /utf-8)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(emergency_search PRIVATE -Wall -Wextra -Wpedantic)
endif()

option(BUILD_TESTS "Build unit tests and benchmarks" ON)

function(add_project_executable name)
    add_executable(${name} ${ARGN})
    set_target_properties(${name} PROPERTIES
        CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
    target_include_directories(${name} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
    if(MSVC)
        target_compile_options(${name} PRIVATE /W4 /utf-8)
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(${name} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endfunction()

function(add_project_test name)
    add_project_executable(${name} ${ARGN})
    add_test(NAME ${name} COMMAND ${name})
endfunction()

if(BUILD_TESTS)
    enable_testing()

    add_project_test(test_hash_table tests/test_hash_table.cpp)
    add_project_test(test_search tests/test_search.cpp src/search/SearchEngine.cpp)
    add_project_test(test_hospital_store tests/test_hospital_store.cpp
        src/data/HospitalStore.cpp src/search/SearchEngine.cpp)
    add_project_executable(bench_search tools/bench_search.cpp
        src/data/HospitalStore.cpp src/search/SearchEngine.cpp)
endif()
