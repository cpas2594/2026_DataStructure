#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

#include "data/CsvLoader.h"
#include "data/HospitalStore.h"

namespace {
void requireTrue(bool ok, const char* expression, const char* file, int line) {
  if (!ok) {
    std::ostringstream message;
    message << file << ':' << line << " CHECK failed: " << expression;
    throw std::runtime_error(message.str());
  }
}
}  // namespace
#define CHECK(expression) \
  requireTrue(static_cast<bool>(expression), #expression, __FILE__, __LINE__)

namespace {

const std::string kHeader =
    "기관ID,주소,병원분류명,응급의료기관코드명,응급실운영여부,기관명,대표전화1,"
    "응급실전화,병원위도\n";

CsvLoadResult loadText(const std::string& text) {
  std::istringstream in(text);
  return CsvLoader::loadFromStream(in);
}

bool hasReason(const CsvLoadResult& r, const std::string& part) {
  for (const CsvRowError& e : r.skipped) {
    if (e.reason.find(part) != std::string::npos) {
      return true;
    }
  }
  return false;
}

void testParsingRules() {
  // BOM, 따옴표 안 쉼표, "" 이스케이프, 따옴표 안 줄바꿈, CRLF, 마지막 줄
  // 줄바꿈 없음
  const std::string text =
      "\xEF\xBB\xBF" + kHeader +
      "A1100011,\"서울특별시 영등포구 63로 10, 여의도성모병원 "
      "(여의도동)\",종합병원,"
      "지역응급의료센터,1,가톨릭대학교여의도성모병원,1661-7575,02-3779-1188,37."
      "5\r\n"
      "A1100002,\"서울특별시 종로구 대학로 101\",종합병원,권역응급의료센터,1,"
      "\"서울\"\"대\"\"병원\",02-1,02-2,37.5\n"
      "A1100003,\"서울특별시 중구\n을지로 "
      "245\",병원,지역응급의료기관,2,국립중앙의료원,02-3,02-4,37.5";
  const CsvLoadResult r = loadText(text);
  CHECK(r.ok);
  CHECK(r.dataRowCount == 3);
  CHECK(r.skipped.empty());
  CHECK(r.hospitals.size() == 3);

  const Hospital& a = r.hospitals[0];
  CHECK(a.id == "A1100011");  // BOM이 첫 열 이름에 붙어도 기관ID를 찾음
  CHECK(a.address == "서울특별시 영등포구 63로 10, 여의도성모병원 (여의도동)");
  CHECK(a.district == "영등포구");
  CHECK(a.hospitalType == "종합병원");
  CHECK(a.emergencyType == "지역응급의료센터");
  CHECK(a.phone == "1661-7575");
  CHECK(a.emergencyPhone == "02-3779-1188");  // CRLF의 CR이 값에 남지 않음
  CHECK(a.emergencyRoomOpen);

  CHECK(r.hospitals[1].name == "서울\"대\"병원");
  CHECK(r.hospitals[2].address == "서울특별시 중구\n을지로 245");
  CHECK(r.hospitals[2].district == "중구");
  CHECK(!r.hospitals[2].emergencyRoomOpen);  // "1"이 아니면 미운영
}

void testValidation() {
  const std::string text =
      kHeader +
      "  A1  , 서울특별시 강남구 도곡로 1 ,종합병원,응급실운영신고기관,1,  "
      "공백병원  ,02,02,0\n"
      "\n"                                                    // 빈 줄: 무시
      "A2,서울특별시 강남구,종합병원,x,1,열부족\n"            // 열 개수 부족
      ",서울특별시 강남구 1,종합병원,x,1,HPID없음,02,02,0\n"  // HPID 없음
      "A4,서울특별시 강남구 1,종합병원,x,1,   ,02,02,0\n"     // 기관명 없음
      "A5,,종합병원,x,1,주소없음,02,02,0\n"                   // 주소 없음
      "A6,경기도 성남시 분당구 1,종합병원,x,1,경기병원,02,02,0\n"  // 자치구
                                                                   // 추출 실패
      "A7,서울특별시 구 1,종합병원,x,1,구만있음,02,02,0\n"  // "구" 한 글자
      "A8,송파구 올림픽로 1,종합병원,x,1,시생략,02,02,0\n"  // 구부터 시작: 허용
      "A9,서울특별시 중구 1, 2층,종합병원,x,1,쉼표밀림,02,02,0\n";  // 따옴표
                                                                    // 없는
                                                                    // 쉼표: 열
                                                                    // 초과
  const CsvLoadResult r = loadText(text);
  CHECK(r.ok);
  CHECK(r.dataRowCount == 9);
  CHECK(r.hospitals.size() == 2);
  CHECK(r.skipped.size() == 7);

  CHECK(r.hospitals[0].id == "A1");  // 앞뒤 공백 제거
  CHECK(r.hospitals[0].name == "공백병원");
  CHECK(r.hospitals[0].address == "서울특별시 강남구 도곡로 1");
  CHECK(r.hospitals[0].district == "강남구");
  CHECK(r.hospitals[1].id == "A8");
  CHECK(r.hospitals[1].district == "송파구");

  CHECK(hasReason(r, "열 개수 불일치"));
  CHECK(hasReason(r, "HPID"));
  CHECK(hasReason(r, "기관명"));
  CHECK(hasReason(r, "주소가 비어"));
  CHECK(hasReason(r, "자치구"));
  CHECK(r.skipped[0].lineNumber == 4);  // 헤더 1, A1 2, 빈 줄 3, 열부족 4
  CHECK(r.skipped[4].hpid == "A6");
  CHECK(r.skipped[6].lineNumber == 11);
  CHECK(r.skipped[6].reason.find("실제 10개") != std::string::npos);
}

void testCrlfLineNumbers() {
  // Windows 줄끝(CRLF)에서도 줄 번호가 두 배로 세지지 않아야 함
  const std::string text =
      kHeader.substr(0, kHeader.size() - 1) + "\r\n" +
      "A1,서울특별시 중구 1,종합병원,x,1,병원1,02,02,0\r\n" +
      "A2,\"서울특별시 중구\r\n2층\",종합병원,x,1,병원2,02,02,0\r\n" +
      "A3,부산광역시 1,종합병원,x,1,병원3,02,02,0\r\n";
  const CsvLoadResult r = loadText(text);
  CHECK(r.ok);
  CHECK(r.dataRowCount == 3);
  CHECK(r.hospitals.size() == 2);
  CHECK(r.hospitals[0].emergencyPhone == "02");
  CHECK(r.skipped.size() == 1);
  CHECK(r.skipped[0].lineNumber == 5);  // 헤더 1, A1 2, A2 3~4, A3 5
}

void testFatalErrors() {
  CHECK(!loadText("").ok);

  const CsvLoadResult missing =
      loadText("기관ID,기관명,주소\nA1,병원,서울특별시 중구 1\n");
  CHECK(!missing.ok);
  CHECK(missing.error.find("병원분류명") != std::string::npos);
  CHECK(missing.error.find("응급실전화") != std::string::npos);
  CHECK(missing.error.find("UTF-8") ==
        std::string::npos);  // 일부만 없으면 인코딩 안내 안 함

  const CsvLoadResult wrongEncoding = loadText(
      "\xB1\xE2\xB0\xFC"
      "ID,x\nA1,y\n");  // CP949 "기관"
  CHECK(!wrongEncoding.ok);
  CHECK(wrongEncoding.error.find("UTF-8") != std::string::npos);

  const CsvLoadResult unterminated =
      loadText(kHeader + "A1,\"서울특별시 중구 1,종합병원,x,1,병원,02,02,0\n");
  CHECK(unterminated.ok);
  CHECK(unterminated.hospitals.empty());
  CHECK(hasReason(unterminated, "따옴표"));

  const CsvLoadResult dupColumn =
      loadText("기관명," + kHeader +
               ",A1,서울특별시 중구 1,종합병원,x,1,병원,02,02,0\n");
  CHECK(!dupColumn.ok);
  CHECK(dupColumn.error.find("같은 이름의 열") != std::string::npos);

  const CsvLoadResult noFile = CsvLoader::loadFile("없는파일.csv");
  CHECK(!noFile.ok);
  CHECK(noFile.error.find("열 수 없습니다") != std::string::npos);
}

void testExtractDistrict() {
  CHECK(CsvLoader::extractDistrict("서울특별시 영등포구 63로 10") ==
        "영등포구");
  CHECK(CsvLoader::extractDistrict("  서울특별시   중구  을지로") == "중구");
  CHECK(CsvLoader::extractDistrict("서울 강서구 공항대로") == "강서구");
  CHECK(CsvLoader::extractDistrict("서울특별시") == "");
  CHECK(CsvLoader::extractDistrict("부산광역시 해운대구 1") == "");
  CHECK(CsvLoader::extractDistrict("") == "");
  // 25개 자치구 목록 기준
  CHECK(CsvLoader::extractDistrict("서울특별시중구 을지로 245") ==
        "중구");  // 붙여 씀
  CHECK(CsvLoader::extractDistrict("(04564) 서울특별시 중구 을지로") ==
        "중구");  // 우편번호 앞
  CHECK(CsvLoader::extractDistrict("서울특별시 중구, 을지로 245") ==
        "중구");  // 구 뒤 쉼표
  CHECK(CsvLoader::extractDistrict("인천광역시 중구 1") ==
        "");  // 다른 시의 중구
  CHECK(CsvLoader::extractDistrict("서울특별시 해운대구 1") ==
        "");  // 서울에 없는 구
  CHECK(CsvLoader::extractDistrict("서울특별시 구 1") == "");
  CHECK(CsvLoader::extractDistrict("Seoul Jung-gu 1") == "");
}

#ifdef EMERGENCY_DATA_DIR
// 실제 공공데이터 파일 검사 (발표자료 수치와 일치하는지 포함)
void testRealFile() {
  const std::string path =
      std::string(EMERGENCY_DATA_DIR) + "/서울시 응급실 위치정보.csv";
  const CsvLoadResult r = CsvLoader::loadFile(path);
  CHECK(r.ok);
  CHECK(r.dataRowCount == 77);
  CHECK(r.skipped.empty());
  CHECK(r.hospitals.size() == 77);
  CHECK(r.hospitals.capacity() == 128);  // 발표 17쪽: 병원 77개 저장 (cap 128)

  // 25개 자치구가 모두 추출되는지
  DynamicArray<std::string> districts;
  for (const Hospital& h : r.hospitals) {
    bool seen = false;
    for (const std::string& d : districts) {
      seen = seen || d == h.district;
    }
    if (!seen) {
      districts.push_back(h.district);
    }
  }
  CHECK(districts.size() == 25);

  // CSV 순서 유지 (발표 9·12·17쪽의 위치 번호)
  CHECK(r.hospitals[19].id == "A1100044");  // 녹색병원
  CHECK(r.hospitals[29].id == "A1100017");  // 서울대학교병원
  CHECK(r.hospitals[60].id == "A1122381");  // 베스티안 성동
  CHECK(r.hospitals[61].id == "A1100018");  // 베스티안 강남
  CHECK(r.hospitals[60].district == "성동구");
  CHECK(r.hospitals[61].district == "강남구");

  // 발표 14쪽: 영등포구 9 -> 종합병원 7 -> 지역응급의료센터 3
  std::size_t gu = 0, general = 0, center = 0;
  for (const Hospital& h : r.hospitals) {
    if (h.district != "영등포구") continue;
    ++gu;
    if (h.hospitalType != "종합병원") continue;
    ++general;
    if (h.emergencyType == "지역응급의료센터") ++center;
  }
  CHECK(gu == 9 && general == 7 && center == 3);

  // 배열 저장소에 적재: 중복 HPID 없음, 위치 번호 그대로
  HospitalStore store;
  for (const Hospital& h : r.hospitals) {
    store.add(h);
  }
  CHECK(store.size() == 77);
  CHECK(store.duplicates().empty());
  CHECK(store.findByName("재단법인베스티안재단베스티안서울병원").size() == 2);
  CHECK(store.idIndex().bucketCount() == 128);  // 발표 9쪽: 128칸
}
#endif

}  // namespace

int main() {
  try {
    testParsingRules();
    testValidation();
    testCrlfLineNumbers();
    testFatalErrors();
    testExtractDistrict();
#ifdef EMERGENCY_DATA_DIR
    testRealFile();
#endif
  } catch (const std::exception& e) {
    std::cerr << "[FAIL] " << e.what() << '\n';
    return 1;
  }
  std::cout << "[PASS] test_csv_loader: BOM, quotes, CRLF, validation, "
               "district, real file 77 rows\n";
  return 0;
}