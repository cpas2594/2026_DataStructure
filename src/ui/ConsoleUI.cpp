#include "ui/ConsoleUI.h"

#include <cstdint>
#include <istream>
#include <ostream>
#include <utility>
#include <vector>

#include "algorithms/MergeSort.hpp"
#include "ui/ResultPrinter.h"

namespace {

const char* const kLine =
    "============================================================";

std::string trim(const std::string& text) {
  std::size_t start = 0;
  while (start < text.size() && (text[start] == ' ' || text[start] == '\t' ||
                                 text[start] == '\r' || text[start] == '\n')) {
    ++start;
  }
  std::size_t end = text.size();
  while (end > start && (text[end - 1] == ' ' || text[end - 1] == '\t' ||
                         text[end - 1] == '\r' || text[end - 1] == '\n')) {
    --end;
  }
  return text.substr(start, end - start);
}

// 올바른 UTF-8인지 확인 (깨진 바이트로 검색하면 글자 중간이 엉뚱하게 일치할 수
// 있음)
bool isValidUtf8(const std::string& text) {
  std::size_t i = 0;
  while (i < text.size()) {
    const unsigned char c = static_cast<unsigned char>(text[i]);
    std::size_t extra = 0;
    unsigned char minNext = 0x80;
    unsigned char maxNext = 0xBF;
    if (c < 0x80) {
      extra = 0;
    } else if (c >= 0xC2 && c <= 0xDF) {
      extra = 1;
    } else if (c >= 0xE0 && c <= 0xEF) {
      extra = 2;
      if (c == 0xE0) minNext = 0xA0;  // 너무 긴 표현 금지
      if (c == 0xED) maxNext = 0x9F;  // UTF-16 대리 영역 금지
    } else if (c >= 0xF0 && c <= 0xF4) {
      extra = 3;
      if (c == 0xF0) minNext = 0x90;
      if (c == 0xF4) maxNext = 0x8F;
    } else {
      return false;
    }
    if (i + extra >= text.size()) {  // 뒤따르는 바이트가 모자람
      return false;
    }
    for (std::size_t k = 1; k <= extra; ++k) {
      const unsigned char next = static_cast<unsigned char>(text[i + k]);
      const unsigned char low = (k == 1) ? minNext : 0x80;
      const unsigned char high = (k == 1) ? maxNext : 0xBF;
      if (next < low || next > high) {
        return false;
      }
    }
    i += extra + 1;
  }
  return true;
}

// 한글 입력기 전각 모드 등으로 들어온 글자를 보통 글자로 바꿈 (올바른 UTF-8에만
// 사용)
//   전각 영문·숫자·기호(！~～) -> 반각, 전각 공백·NBSP -> 공백, 폭 없는
//   공백·BOM -> 삭제
std::string normalizeInput(const std::string& text) {
  std::string out;
  out.reserve(text.size());
  std::size_t i = 0;
  while (i < text.size()) {
    const unsigned char c = static_cast<unsigned char>(text[i]);
    const std::size_t len = c < 0x80 ? 1 : (c < 0xE0 ? 2 : (c < 0xF0 ? 3 : 4));
    std::uint32_t cp = (len == 1) ? c : (c & (0xFF >> (len + 1)));
    for (std::size_t k = 1; k < len; ++k) {
      cp = (cp << 6) | (static_cast<unsigned char>(text[i + k]) & 0x3F);
    }
    if (cp >= 0xFF01 && cp <= 0xFF5E) {
      out += static_cast<char>(cp - 0xFEE0);
    } else if (cp == 0x3000 || cp == 0x00A0) {
      out += ' ';
    } else if (cp == 0x200B || cp == 0xFEFF) {
      // 보이지 않는 글자는 버림
    } else {
      out.append(text, i, len);
    }
    i += len;
  }
  return out;
}

// "12"처럼 숫자로만 된 입력을 1 이상 max 이하의 번호로 변환. 아니면 0
std::size_t parseNumber(const std::string& text, std::size_t max) {
  if (text.empty() || text.size() > 9) {
    return 0;
  }
  std::size_t value = 0;
  for (char c : text) {
    if (c < '0' || c > '9') {
      return 0;
    }
    value = value * 10 + static_cast<std::size_t>(c - '0');
  }
  return (value >= 1 && value <= max) ? value : 0;
}

bool contains(const DynamicArray<std::string>& values,
              const std::string& value) {
  for (const std::string& v : values) {
    if (v == value) {
      return true;
    }
  }
  return false;
}

// 빈 값을 뺀 중복 없는 값 목록을 가나다순(UTF-8 바이트 순)으로
DynamicArray<std::string> collectOptions(const std::vector<Hospital>& hospitals,
                                         std::string Hospital::* field) {
  DynamicArray<std::string> options;
  for (const Hospital& h : hospitals) {
    const std::string& value = h.*field;
    if (!value.empty() && !contains(options, value)) {
      options.push_back(value);
    }
  }
  mergeSort(options,
            [](const std::string& a, const std::string& b) { return a < b; });
  return options;
}

const char* matchTypeLabel(MatchType type) {
  switch (type) {
    case MatchType::All:
      return "전체 목록";
    case MatchType::ExactId:
      return "HPID 일치";
    case MatchType::ExactName:
      return "병원명 완전 일치";
    case MatchType::PartialName:
      return "병원명 부분 일치";
  }
  return "";
}

const std::string& orAll(const std::string& value) {
  static const std::string kAll = "전체";
  return value.empty() ? kAll : value;
}

}  // namespace

ConsoleUI::ConsoleUI(const SearchService& service, const HospitalStore& store,
                     std::istream& in, std::ostream& out, LineReader reader)
    : service_(service),
      out_(out),
      reader_(std::move(reader)),
      hospitalCount_(store.size()) {
  if (!reader_) {
    reader_ = [&in](std::string& line) {
      return static_cast<bool>(std::getline(in, line));
    };
  }
  districts_ = collectOptions(store.hospitals(), &Hospital::district);
  hospitalTypes_ = collectOptions(store.hospitals(), &Hospital::hospitalType);
  emergencyTypes_ = collectOptions(store.hospitals(), &Hospital::emergencyType);
}

bool ConsoleUI::readLine(const char* prompt, std::string& line) {
  if (prompt[0] != '\0') {
    out_ << prompt << '\n';
  }
  out_ << "> " << std::flush;
  while (reader_(line)) {
    // 메모장 등에서 "UTF-8(BOM)"으로 저장한 입력 파일의 첫 줄 앞 3바이트 제거
    if (line.compare(0, 3, "\xEF\xBB\xBF") == 0) {
      line.erase(0, 3);
    }
    if (!isValidUtf8(line)) {
      out_ << "입력한 글자를 읽을 수 없습니다 (UTF-8이 아님). 다시 "
              "입력하세요.\n> "
           << std::flush;
      continue;
    }
    line = trim(normalizeInput(line));
    return true;
  }
  out_ << '\n';
  return false;
}

void ConsoleUI::run() {
  printWelcome(hospitalCount_);
  while (true) {
    SearchQuery query;
    if (!askQuery(query)) {
      break;
    }
    const SearchOutcome outcome = service_.search(query);
    printQuerySummary(query, outcome);
    ResultPrinter::printList(out_, outcome.hospitals);
    if (outcome.hospitals.empty() && outcome.candidateCount > 0) {
      out_ << "* 검색어로 " << outcome.candidateCount
           << "곳을 찾았지만 선택한 조건에 맞는 곳이 없습니다.\n";
    }
    if (!outcome.hospitals.empty() && !askDetails(outcome.hospitals)) {
      break;
    }
    if (!askSearchAgain()) {
      break;
    }
  }
  out_ << "프로그램을 종료합니다.\n";
}

bool ConsoleUI::askQuery(SearchQuery& query) {
  out_ << '\n' << kLine << '\n';
  std::string line;
  if (!readLine(
          "[1/4] 검색어: 병원명(일부도 가능) 또는 HPID  (Enter: 전체, q: 종료)",
          line)) {
    return false;
  }
  if (line == "q" || line == "Q") {
    return false;
  }
  query.keyword = line;
  return askOption("[2/4] 자치구", districts_, query.district) &&
         askOption("[3/4] 병원 종류", hospitalTypes_, query.hospitalType) &&
         askOption("[4/4] 응급의료기관 종류", emergencyTypes_,
                   query.emergencyType);
}

bool ConsoleUI::askOption(const char* title,
                          const DynamicArray<std::string>& options,
                          std::string& selected) {
  out_ << title << "  (번호 또는 이름 입력, Enter: 선택 안 함)\n";
  for (std::size_t i = 0; i < options.size(); ++i) {
    out_ << (i + 1 < 10 ? "   " : "  ") << (i + 1) << '.' << options[i];
    out_ << ((i + 1) % 5 == 0 || i + 1 == options.size() ? "\n" : "   ");
  }

  std::string line;
  while (readLine("", line)) {
    if (line.empty()) {
      selected.clear();
      return true;
    }
    const std::size_t number = parseNumber(line, options.size());
    if (number != 0) {
      selected = options[number - 1];
      return true;
    }
    if (contains(options, line)) {
      selected = line;
      return true;
    }
    out_ << "1~" << options.size()
         << " 사이 번호나 목록에 있는 이름을 입력하세요.\n";
  }
  return false;
}

bool ConsoleUI::askDetails(const DynamicArray<Hospital>& results) {
  const std::string prompt = "상세 정보를 볼 번호 (1~" +
                             std::to_string(results.size()) +
                             ", Enter: 건너뛰기)";
  std::string line;
  while (readLine(prompt.c_str(), line)) {
    if (line.empty()) {
      return true;
    }
    const std::size_t number = parseNumber(line, results.size());
    if (number == 0) {
      out_ << "1~" << results.size() << " 사이 번호를 입력하세요.\n";
      continue;
    }
    ResultPrinter::printDetail(out_, results[number - 1], number);
  }
  return false;
}

bool ConsoleUI::askSearchAgain() {
  std::string line;
  while (readLine("다시 검색할까요? (Enter 또는 y: 예, n: 종료)", line)) {
    if (line.empty() || line == "y" || line == "Y") {
      return true;
    }
    if (line == "n" || line == "N") {
      return false;
    }
    out_ << "y 또는 n을 입력하세요.\n";
  }
  return false;
}

void ConsoleUI::printWelcome(std::size_t hospitalCount) const {
  out_ << kLine << '\n'
       << "  서울시 응급실 종합 검색 시스템\n"
       << "  병원 " << hospitalCount << "곳 · 자치구 " << districts_.size()
       << "개\n"
       << "  * 공공데이터 기준 정적 정보이며 실시간 병상 현황이 아닙니다.\n";
}

void ConsoleUI::printQuerySummary(const SearchQuery& query,
                                  const SearchOutcome& outcome) const {
  out_ << '\n'
       << "검색어: " << (query.keyword.empty() ? "(전체)" : query.keyword)
       << "  [" << matchTypeLabel(outcome.matchType) << ", 후보 "
       << outcome.candidateCount << "곳]\n"
       << "조건: 자치구 " << orAll(query.district) << " · 병원 종류 "
       << orAll(query.hospitalType) << " · 응급기관 "
       << orAll(query.emergencyType) << '\n';
}