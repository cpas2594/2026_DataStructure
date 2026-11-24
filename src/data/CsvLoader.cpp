#include "data/CsvLoader.h"

#include <filesystem>
#include <fstream>
#include <istream>
#include <string>

namespace {

const char* const kUtf8Bom = "\xEF\xBB\xBF";

bool isSpace(char c) { return c == ' ' || c == '\t' || c == '\r' || c == '\n'; }

// 서울을 뜻하는 주소 첫 단어인지 확인
bool isSeoulPrefix(const std::string& word) {
  return word == "서울특별시" || word == "서울시" || word == "서울";
}

// 서울시 자치구 25개 (이 목록에 있는 이름만 자치구로 인정)
const char* const kSeoulDistricts[] = {
    "강남구", "강동구", "강북구", "강서구",   "관악구", "광진구",   "구로구",
    "금천구", "노원구", "도봉구", "동대문구", "동작구", "마포구",   "서대문구",
    "서초구", "성동구", "성북구", "송파구",   "양천구", "영등포구", "용산구",
    "은평구", "종로구", "중구",   "중랑구",
};

bool isSeoulDistrict(const std::string& word) {
  for (const char* district : kSeoulDistricts) {
    if (word == district) {
      return true;
    }
  }
  return false;
}

// 단어 앞뒤의 괄호·쉼표·마침표 제거: "(04564)" "중구," 같은 경우
std::string stripPunctuation(const std::string& word) {
  const std::string marks = "()[],.;:";
  std::size_t start = 0;
  std::size_t end = word.size();
  while (start < end && marks.find(word[start]) != std::string::npos) {
    ++start;
  }
  while (end > start && marks.find(word[end - 1]) != std::string::npos) {
    --end;
  }
  return word.substr(start, end - start);
}

// 공백으로 구분된 단어를 앞에서부터 최대 maxWords개 꺼냄
DynamicArray<std::string> firstWords(const std::string& text,
                                     std::size_t maxWords) {
  DynamicArray<std::string> words;
  std::size_t i = 0;
  while (i < text.size() && words.size() < maxWords) {
    while (i < text.size() && isSpace(text[i])) {
      ++i;
    }
    const std::size_t start = i;
    while (i < text.size() && !isSpace(text[i])) {
      ++i;
    }
    if (i > start) {
      words.push_back(text.substr(start, i - start));
    }
  }
  return words;
}

// 헤더에서 열 이름의 위치를 찾음. 없으면 false
bool findColumn(const DynamicArray<std::string>& header, const char* name,
                std::size_t& index) {
  for (std::size_t i = 0; i < header.size(); ++i) {
    if (header[i] == name) {
      index = i;
      return true;
    }
  }
  return false;
}

void addSkipped(CsvLoadResult& result, std::size_t line,
                const std::string& hpid, const std::string& reason) {
  result.skipped.push_back(CsvRowError{line, hpid, reason});
}

// 필수 열 위치
struct ColumnIndexes {
  std::size_t id = 0;
  std::size_t name = 0;
  std::size_t address = 0;
  std::size_t hospitalType = 0;
  std::size_t emergencyType = 0;
  std::size_t phone = 0;
  std::size_t emergencyPhone = 0;
  std::size_t emergencyOpen = 0;
};

}  // namespace

std::string CsvLoader::trim(const std::string& text) {
  std::size_t start = 0;
  while (start < text.size() && isSpace(text[start])) {
    ++start;
  }
  std::size_t end = text.size();
  while (end > start && isSpace(text[end - 1])) {
    --end;
  }
  return text.substr(start, end - start);
}

std::string CsvLoader::extractDistrict(const std::string& address) {
  // 앞쪽 몇 단어만 봄: "(우편번호) 서울특별시 중구 ..."까지 허용
  const DynamicArray<std::string> words = firstWords(address, 4);
  const char* const prefixes[] = {"서울특별시", "서울시", "서울"};
  for (std::size_t i = 0; i < words.size(); ++i) {
    const std::string word = stripPunctuation(words[i]);

    // "서울특별시중구"처럼 시 이름과 구가 붙어 있는 경우
    for (const char* prefix : prefixes) {
      const std::string p = prefix;
      if (word.size() > p.size() && word.compare(0, p.size(), p) == 0 &&
          isSeoulDistrict(word.substr(p.size()))) {
        return word.substr(p.size());
      }
    }

    if (isSeoulDistrict(word)) {
      // 구부터 시작하는 주소이거나, 바로 앞 단어가 서울이어야 함
      // ("인천광역시 중구"의 중구는 서울 중구가 아님)
      if (i == 0 || isSeoulPrefix(stripPunctuation(words[i - 1]))) {
        return word;
      }
      return "";
    }
  }
  return "";
}

bool CsvLoader::readRecord(std::istream& in, DynamicArray<std::string>& fields,
                           std::size_t& lineNumber, bool& unterminatedQuote) {
  fields = DynamicArray<std::string>();
  unterminatedQuote = false;
  if (in.peek() == std::char_traits<char>::eof()) {
    return false;
  }

  std::string field;
  bool inQuotes = false;
  int c;
  while ((c = in.get()) != std::char_traits<char>::eof()) {
    const char ch = static_cast<char>(c);
    if (inQuotes) {
      if (ch == '"') {
        if (in.peek() == '"') {  // "" 는 따옴표 문자 하나
          in.get();
          field += '"';
        } else {
          inQuotes = false;
        }
      } else {
        if (ch == '\n') {  // 따옴표 안 줄바꿈은 값의 일부
          ++lineNumber;
        }
        field += ch;
      }
      continue;
    }

    if (ch == '"') {
      inQuotes = true;
    } else if (ch == ',') {
      fields.push_back(field);
      field.clear();
    } else if (ch == '\r' || ch == '\n') {
      if (ch == '\r' && in.peek() == '\n') {
        in.get();
      }
      ++lineNumber;
      fields.push_back(field);
      return true;
    } else {
      field += ch;
    }
  }

  // 마지막 줄에 줄바꿈이 없는 경우
  unterminatedQuote = inQuotes;
  fields.push_back(field);
  ++lineNumber;
  return true;
}

CsvLoadResult CsvLoader::loadFromStream(std::istream& in) {
  CsvLoadResult result;
  std::size_t lineNumber = 1;
  bool unterminated = false;

  // 1. 헤더 읽기
  DynamicArray<std::string> header;
  if (!readRecord(in, header, lineNumber, unterminated)) {
    result.error = "CSV 파일이 비어 있습니다.";
    return result;
  }
  if (unterminated) {
    result.error = "헤더의 따옴표가 닫히지 않았습니다.";
    return result;
  }
  if (header.size() > 0 && header[0].compare(0, 3, kUtf8Bom) == 0) {
    header[0] = header[0].substr(3);  // UTF-8 BOM 제거
  }
  for (std::size_t i = 0; i < header.size(); ++i) {
    header[i] = trim(header[i]);
  }

  // 2. 필수 열 위치 찾기
  ColumnIndexes col;
  struct Required {
    const char* name;
    std::size_t* index;
  };
  const Required required[] = {
      {kColumnId, &col.id},
      {kColumnName, &col.name},
      {kColumnAddress, &col.address},
      {kColumnHospitalType, &col.hospitalType},
      {kColumnEmergencyType, &col.emergencyType},
      {kColumnPhone, &col.phone},
      {kColumnEmergencyPhone, &col.emergencyPhone},
      {kColumnEmergencyOpen, &col.emergencyOpen},
  };
  // 필수 열 이름이 두 번 이상 나오면 어느 쪽을 써야 할지 알 수 없으므로 중단
  for (const Required& r : required) {
    std::size_t count = 0;
    for (const std::string& name : header) {
      count += (name == r.name) ? 1 : 0;
    }
    if (count > 1) {
      result.error =
          std::string("같은 이름의 열이 여러 개 있습니다: ") + r.name;
      return result;
    }
  }

  std::string missing;
  std::size_t missingCount = 0;
  for (const Required& r : required) {
    if (!findColumn(header, r.name, *r.index)) {
      missing += missing.empty() ? "" : ", ";
      missing += r.name;
      ++missingCount;
    }
  }
  if (missingCount > 0) {
    result.error = "필수 열이 없습니다: " + missing;
    if (missingCount == sizeof(required) / sizeof(required[0])) {
      // 하나도 못 찾으면 대부분 인코딩 문제 (서울시 원본 다운로드는 CP949)
      result.error += " (파일이 UTF-8인지 확인하세요)";
    }
    return result;
  }
  const std::size_t columnCount = header.size();

  // 3. 데이터 행마다 검증 후 적재
  DynamicArray<std::string> fields;
  while (true) {
    const std::size_t rowLine = lineNumber;
    if (!readRecord(in, fields, lineNumber, unterminated)) {
      break;
    }
    if (fields.size() == 1 && trim(fields[0]).empty() && !unterminated) {
      continue;  // 빈 줄은 데이터 행으로 세지 않음
    }
    ++result.dataRowCount;

    if (unterminated) {
      addSkipped(result, rowLine, "", "따옴표가 닫히지 않은 채 파일이 끝남");
      break;
    }
    if (fields.size() != columnCount) {
      addSkipped(result, rowLine, "",
                 "열 개수 불일치 (기대 " + std::to_string(columnCount) +
                     "개, 실제 " + std::to_string(fields.size()) + "개)");
      continue;
    }

    Hospital h{};
    h.id = trim(fields[col.id]);
    h.name = trim(fields[col.name]);
    h.address = trim(fields[col.address]);
    h.hospitalType = trim(fields[col.hospitalType]);
    h.emergencyType = trim(fields[col.emergencyType]);
    h.phone = trim(fields[col.phone]);
    h.emergencyPhone = trim(fields[col.emergencyPhone]);
    h.emergencyRoomOpen = trim(fields[col.emergencyOpen]) == "1";

    if (h.id.empty()) {
      addSkipped(result, rowLine, "", "HPID(기관ID)가 비어 있음");
      continue;
    }
    if (h.name.empty()) {
      addSkipped(result, rowLine, h.id, "기관명이 비어 있음");
      continue;
    }
    if (h.address.empty()) {
      addSkipped(result, rowLine, h.id, "주소가 비어 있음");
      continue;
    }
    h.district = extractDistrict(h.address);
    if (h.district.empty()) {
      addSkipped(result, rowLine, h.id,
                 "주소에서 자치구를 찾을 수 없음: " + h.address);
      continue;
    }

    result.hospitals.push_back(h);
  }

  result.ok = true;
  return result;
}

CsvLoadResult CsvLoader::loadFile(const std::string& path) {
  // u8path: UTF-8 경로를 운영체제 형식으로 변환 (Windows 한글 파일명 대응)
  std::ifstream file(std::filesystem::u8path(path), std::ios::binary);
  if (!file) {
    CsvLoadResult result;
    result.error = "CSV 파일을 열 수 없습니다: " + path;
    return result;
  }
  return loadFromStream(file);
}