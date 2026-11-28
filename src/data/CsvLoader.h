#pragma once

// 역할: CSV 로딩 기능과 로딩 결과 형식의 선언.
// TODO: 읽기 함수, 검증 결과, 제외 행과 오류 사유를 전달할 형식 선언.
#pragma once

// 역할: 서울시 응급실 위치정보 CSV를 읽어 검증·정제한 뒤 병원 배열로 돌려줌
// 흐름: 파일 열기 -> BOM 제거 -> 헤더에서 필수 열 위치 찾기
//       -> 행마다 [열 개수 확인 -> 앞뒤 공백 제거 -> 필수 값 확인 -> 주소에서
//       자치구 추출]
//       -> 통과한 행만 CSV 순서대로 hospitals에 push_back
// HPID 중복 제외는 HospitalStore::add()가 담당 (첫 번째 유효 행 유지 + 중복
// 기록)

#include <cstddef>
#include <iosfwd>
#include <string>

#include "model/Hospital.h"
#include "structures/DynamicArray.hpp"

// 제외된 행 하나의 기록
struct CsvRowError {
  std::size_t lineNumber;  // 파일에서 그 행이 시작하는 줄 번호 (헤더가 1번 줄)
  std::string hpid;        // 읽을 수 있었으면 HPID, 아니면 빈 문자열
  std::string reason;      // 제외 사유
};

struct CsvLoadResult {
  bool ok = false;                    // 파일 열기와 헤더 검사를 통과했는지
  std::string error;                  // ok == false일 때의 원인
  std::size_t dataRowCount = 0;       // 헤더를 뺀 데이터 행 수 (빈 줄 제외)
  DynamicArray<Hospital> hospitals;   // 검증을 통과한 병원 (CSV 순서 유지)
  DynamicArray<CsvRowError> skipped;  // 제외된 행과 사유
};

class CsvLoader {
 public:
  // 반드시 있어야 하는 CSV 열 이름 (공식 데이터 OA-20338 기준)
  static constexpr const char* kColumnId = "기관ID";
  static constexpr const char* kColumnName = "기관명";
  static constexpr const char* kColumnAddress = "주소";
  static constexpr const char* kColumnHospitalType = "병원분류명";
  static constexpr const char* kColumnEmergencyType = "응급의료기관코드명";
  static constexpr const char* kColumnPhone = "대표전화1";
  static constexpr const char* kColumnEmergencyPhone = "응급실전화";
  static constexpr const char* kColumnEmergencyOpen = "응급실운영여부";

  // path는 UTF-8 문자열 (Windows에서도 한글 파일명을 열 수 있도록 처리)
  static CsvLoadResult loadFile(const std::string& path);

  // 이미 열린 입력에서 읽기 (테스트에서 문자열로 CSV를 넣을 때 사용)
  static CsvLoadResult loadFromStream(std::istream& in);

  // CSV 레코드 하나를 읽어 fields에 담음
  // 따옴표 안의 쉼표·줄바꿈, "" 이스케이프, CRLF/LF 줄끝을 처리
  // 반환: 레코드를 읽었으면 true, 더 읽을 것이 없으면 false
  // lineNumber: 다음에 읽을 줄 번호. 읽은 줄 수만큼 증가
  // unterminatedQuote: 따옴표가 닫히지 않은 채 입력이 끝났으면 true
  static bool readRecord(std::istream& in, DynamicArray<std::string>& fields,
                         std::size_t& lineNumber, bool& unterminatedQuote);

  // "서울특별시 영등포구 63로 10, ..." -> "영등포구". 찾지 못하면 빈 문자열
  static std::string extractDistrict(const std::string& address);

  // 앞뒤 공백(스페이스, 탭, CR, LF) 제거
  static std::string trim(const std::string& text);
};