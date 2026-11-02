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

# tests/는 주석만 있는 자리입니다. 실제 테스트 작성 후 별도 대상으로 추가합니다.
