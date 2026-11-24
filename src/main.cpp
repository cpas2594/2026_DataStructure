// 프로그램 진입점
// 순서 (발표자료 5쪽): CSV 로딩 -> 저장소(색인) 생성 -> 검색 서비스 연결 ->
// 콘솔 UI 실행

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

#include "data/CsvLoader.h"
#include "data/HospitalStore.h"
#include "search/SearchService.h"
#include "ui/ConsoleUI.h"

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace {

const char* const kCsvFileName = "서울시 응급실 위치정보.csv";

#ifdef _WIN32
// Windows 콘솔에서 한글 한 줄을 UTF-16으로 읽어 UTF-8로 변환
bool readConsoleLine(std::string& line) {
  HANDLE input = GetStdHandle(STD_INPUT_HANDLE);
  std::wstring wide;
  wchar_t buffer[256];
  while (true) {
    DWORD count = 0;
    if (!ReadConsoleW(input, buffer, 255, &count, nullptr) || count == 0) {
      if (wide.empty()) {
        return false;
      }
      break;
    }
    wide.append(buffer, count);
    if (wide.back() == L'\n') {
      break;
    }
  }
  while (!wide.empty() && (wide.back() == L'\n' || wide.back() == L'\r')) {
    wide.pop_back();
  }
  if (!wide.empty() && wide[0] == 0x1A) {  // Ctrl+Z: 입력 종료
    return false;
  }
  const int size = WideCharToMultiByte(CP_UTF8, 0, wide.c_str(),
                                       static_cast<int>(wide.size()), nullptr,
                                       0, nullptr, nullptr);
  line.assign(static_cast<std::size_t>(size), '\0');
  if (size > 0) {
    WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), static_cast<int>(wide.size()),
                        &line[0], size, nullptr, nullptr);
  }
  return true;
}

bool isConsoleInput() {
  DWORD mode = 0;
  return GetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), &mode) != 0;
}
#endif

bool fileExists(const std::string& path) {
  return static_cast<bool>(std::ifstream(std::filesystem::u8path(path)));
}

// CSV 위치 찾기: 실행 인자 -> 현재 폴더 기준 data/ -> 빌드 때 기록한 프로젝트
// data/
std::string findCsvPath(int argc, char* argv[]) {
  if (argc > 1) {
    return argv[1];
  }
  const std::string local = std::string("data/") + kCsvFileName;
  if (fileExists(local)) {
    return local;
  }
#ifdef EMERGENCY_DATA_DIR
  return std::string(EMERGENCY_DATA_DIR) + "/" + kCsvFileName;
#else
  return local;
#endif
}

}  // namespace

int main(int argc, char* argv[]) {
#ifdef _WIN32
  SetConsoleOutputCP(CP_UTF8);
  SetConsoleCP(CP_UTF8);
#endif

  // 1. CSV 로딩
  const std::string path = findCsvPath(argc, argv);
  const CsvLoadResult loaded = CsvLoader::loadFile(path);
  if (!loaded.ok) {
    std::cout << "[오류] " << loaded.error << '\n';
    return 1;
  }

  // 2. 저장소와 색인 생성 (store는 프로그램이 끝날 때까지 유지)
  HospitalStore store;
  for (const Hospital& hospital : loaded.hospitals) {
    store.add(hospital);
  }
  if (!loaded.skipped.empty() || !store.duplicates().empty()) {
    std::cout << "[알림] 제외된 행 " << loaded.skipped.size()
              << "개, 중복 HPID " << store.duplicates().size() << "개\n";
    for (const CsvRowError& e : loaded.skipped) {
      std::cout << "  - " << e.lineNumber << "번 줄: " << e.reason << '\n';
    }
  }
  if (store.empty()) {
    std::cout << "[오류] 불러온 병원이 없습니다: " << path << '\n';
    return 1;
  }

  // 3. 검색 서비스 연결, 4. 콘솔 UI 실행
  const SearchService service(store);
  ConsoleUI::LineReader reader = nullptr;
#ifdef _WIN32
  if (isConsoleInput()) {
    reader =
        readConsoleLine;  // 키보드 입력이면 한글이 깨지지 않는 방식으로 읽음
  }
#endif
  ConsoleUI ui(service, store, std::cin, std::cout, reader);
  ui.run();
  return 0;
}