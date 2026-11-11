#include "model/Hospital.h"
#include "search/HospitalFilter.h"

#include <cstddef>
#include <exception>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
void requireTrue(bool ok, const char* expression, int line) {
    if (!ok) {
        std::ostringstream message;
        message << __FILE__ << ':' << line << " CHECK failed: " << expression;
        throw std::runtime_error(message.str());
    }
}

#define CHECK(expression) requireTrue(static_cast<bool>(expression), #expression, __LINE__)

Hospital makeHospital(const std::string& id, const std::string& name,
                      const std::string& district, const std::string& hospitalType,
                      const std::string& emergencyType) {
    Hospital hospital{};
    hospital.id = id;
    hospital.name = name;
    hospital.district = district;
    hospital.hospitalType = hospitalType;
    hospital.emergencyType = emergencyType;
    return hospital;
}

void checkIds(const std::vector<Hospital>& result,
              const std::vector<std::string>& expected) {
    CHECK(result.size() == expected.size());
    for (std::size_t i = 0; i < expected.size(); ++i) {
        CHECK(result[i].id == expected[i]);
    }
}

void checkHospitals(const std::vector<Hospital>& actual,
                   const std::vector<Hospital>& expected) {
    CHECK(actual.size() == expected.size());
    for (std::size_t i = 0; i < expected.size(); ++i) {
        CHECK(actual[i].id == expected[i].id);
        CHECK(actual[i].name == expected[i].name);
        CHECK(actual[i].district == expected[i].district);
        CHECK(actual[i].hospitalType == expected[i].hospitalType);
        CHECK(actual[i].emergencyType == expected[i].emergencyType);
    }
}

// 실제 CSV 행에서 확인한 값입니다. 자치구는 주소에서 확인했습니다.
// CSV 파싱과 자치구 추출 자체는 데이터 모듈에서 검증합니다.
std::vector<Hospital> makeCsvSamples() {
    return {
        makeHospital("A1100011", "가톨릭대학교여의도성모병원", "영등포구",
                     "종합병원", "지역응급의료센터"),
        makeHospital("A1100026", "구로성심병원", "구로구",
                     "종합병원", "지역응급의료기관"),
        makeHospital("A1100043", "강동경희대학교병원", "강동구",
                     "종합병원", "권역응급의료센터"),
        makeHospital("A1100047", "강남고려병원", "관악구",
                     "병원", "응급실운영신고기관"),
        makeHospital("A1107402", "서울대학교치과병원", "종로구",
                     "치과병원", "응급실운영신고기관")
    };
}

// 각 조건의 일치/불일치 조합 8개를 만드는 가상 데이터입니다.
// 이름을 같게 두어 이름에 의한 중복 제거가 발생하지 않는지도 확인합니다.
std::vector<Hospital> makeCombinationSamples() {
    std::vector<Hospital> hospitals;
    for (int mask = 0; mask < 8; ++mask) {
        hospitals.push_back(makeHospital(
            "T" + std::to_string(mask), "테스트병원",
            (mask & 1) ? "영등포구" : "관악구",
            (mask & 2) ? "종합병원" : "병원",
            (mask & 4) ? "지역응급의료센터" : "지역응급의료기관"
        ));
    }
    return hospitals;
}

std::vector<FilterConditions> makeConditionCombinations() {
    return {
        {"", "", ""},
        {"영등포구", "", ""},
        {"", "종합병원", ""},
        {"영등포구", "종합병원", ""},
        {"", "", "지역응급의료센터"},
        {"영등포구", "", "지역응급의료센터"},
        {"", "종합병원", "지역응급의료센터"},
        {"영등포구", "종합병원", "지역응급의료센터"}
    };
}

void testDistrictFilter() {
    const auto hospitals = makeCsvSamples();
    checkIds(HospitalFilter::apply(hospitals, {"영등포구", "", ""}), {"A1100011"});
    // 병원 이름의 '강남'이 아닌 주소에서 얻은 관악구로 판정해야 합니다.
    checkIds(HospitalFilter::apply(hospitals, {"관악구", "", ""}), {"A1100047"});
}

void testHospitalTypeFilter() {
    const auto hospitals = makeCsvSamples();
    checkIds(HospitalFilter::apply(hospitals, {"", "종합병원", ""}),
             {"A1100011", "A1100026", "A1100043"});
    checkIds(HospitalFilter::apply(hospitals, {"", "병원", ""}), {"A1100047"});
    checkIds(HospitalFilter::apply(hospitals, {"", "치과병원", ""}), {"A1107402"});
}

void testEmergencyTypeFilter() {
    const auto hospitals = makeCsvSamples();
    checkIds(HospitalFilter::apply(hospitals, {"", "", "지역응급의료센터"}), {"A1100011"});
    checkIds(HospitalFilter::apply(hospitals, {"", "", "지역응급의료기관"}), {"A1100026"});
    checkIds(HospitalFilter::apply(hospitals, {"", "", "권역응급의료센터"}), {"A1100043"});
    checkIds(HospitalFilter::apply(hospitals, {"", "", "응급실운영신고기관"}),
             {"A1100047", "A1107402"});
}

void testAllConditionCombinations() {
    const auto hospitals = makeCombinationSamples();
    const auto conditions = makeConditionCombinations();
    const std::vector<std::vector<std::string>> expected = {
        {"T0", "T1", "T2", "T3", "T4", "T5", "T6", "T7"},
        {"T1", "T3", "T5", "T7"},
        {"T2", "T3", "T6", "T7"},
        {"T3", "T7"},
        {"T4", "T5", "T6", "T7"},
        {"T5", "T7"},
        {"T6", "T7"},
        {"T7"}
    };
    for (std::size_t i = 0; i < conditions.size(); ++i) {
        checkIds(HospitalFilter::apply(hospitals, conditions[i]), expected[i]);
    }
}

void testEachSelectedConditionMustMatch() {
    const auto hospitals = makeCombinationSamples();
    checkIds(HospitalFilter::apply(hospitals, {"없는구", "종합병원", "지역응급의료센터"}), {});
    checkIds(HospitalFilter::apply(hospitals, {"영등포구", "없는종류", "지역응급의료센터"}), {});
    checkIds(HospitalFilter::apply(hospitals, {"영등포구", "종합병원", "없는기관"}), {});
}

void testEmptyAndSingleCandidate() {
    const auto hospitals = makeCombinationSamples();
    const std::vector<Hospital> single = {hospitals[7]};
    for (const auto& conditions : makeConditionCombinations()) {
        checkIds(HospitalFilter::apply({}, conditions), {});
        checkIds(HospitalFilter::apply(single, conditions), {"T7"});
    }
    checkIds(HospitalFilter::apply(single, {"관악구", "", ""}), {});
}

void testMissingFields() {
    const std::vector<Hospital> hospitals = {
        makeHospital("D", "지역누락", "", "종합병원", "지역응급의료센터"),
        makeHospital("H", "병원종류누락", "영등포구", "", "지역응급의료센터"),
        makeHospital("E", "기관종류누락", "영등포구", "종합병원", "")
    };
    checkIds(HospitalFilter::apply(hospitals, {}), {"D", "H", "E"});
    checkIds(HospitalFilter::apply(hospitals, {"영등포구", "", ""}), {"H", "E"});
    checkIds(HospitalFilter::apply(hospitals, {"", "종합병원", ""}), {"D", "E"});
    checkIds(HospitalFilter::apply(hospitals, {"", "", "지역응급의료센터"}), {"D", "H"});
    checkIds(HospitalFilter::apply(hospitals, {"영등포구", "종합병원", "지역응급의료센터"}), {});
}

void testCandidateScopeAndOrder() {
    const auto hospitals = makeCombinationSamples();
    const std::vector<Hospital> candidates = {hospitals[7], hospitals[3], hospitals[1]};
    checkIds(HospitalFilter::apply(candidates, {}), {"T7", "T3", "T1"});
    checkIds(HospitalFilter::apply(candidates, {"", "종합병원", ""}), {"T7", "T3"});
    checkIds(HospitalFilter::apply(candidates, {"영등포구", "종합병원", "지역응급의료센터"}), {"T7"});
}

void testExactMatching() {
    const auto hospitals = makeCsvSamples();
    checkIds(HospitalFilter::apply(hospitals, {"영등포", "", ""}), {});
    checkIds(HospitalFilter::apply(hospitals, {"", "종합", ""}), {});
    checkIds(HospitalFilter::apply(hospitals, {"", "", "응급의료센터"}), {});
    // 입력 공백 정리는 호출 측 책임이며 필터는 완전 일치만 수행합니다.
    checkIds(HospitalFilter::apply(hospitals, {" 영등포구 ", "", ""}), {});
    checkIds(HospitalFilter::apply(hospitals, {"", " 병원 ", ""}), {});
    checkIds(HospitalFilter::apply(hospitals, {"", "", " 지역응급의료센터 "}), {});
}

void testInputPreservationAndResultCopies() {
    const auto candidates = makeCombinationSamples();
    const auto before = candidates;
    for (const auto& conditions : makeConditionCombinations()) {
        HospitalFilter::apply(candidates, conditions);
        checkHospitals(candidates, before);
    }

    auto result = HospitalFilter::apply(candidates, {});
    checkHospitals(result, before);
    result[0].id = "변경ID";
    result[0].name = "변경이름";
    result[0].district = "변경지역";
    result[0].hospitalType = "변경병원종류";
    result[0].emergencyType = "변경기관종류";
    checkHospitals(candidates, before);
}

void runTest(const char* name, void (*test)()) {
    test();
    std::cout << "[PASS] " << name << '\n';
}
}  // namespace

int main() {
    try {
        runTest("district filter (CSV samples)", testDistrictFilter);
        runTest("hospital types (CSV samples)", testHospitalTypeFilter);
        runTest("emergency institution types (CSV samples)", testEmergencyTypeFilter);
        runTest("all 8 condition combinations", testAllConditionCombinations);
        runTest("each selected condition must match", testEachSelectedConditionMustMatch);
        runTest("empty and single candidate", testEmptyAndSingleCandidate);
        runTest("missing hospital fields", testMissingFields);
        runTest("candidate scope, duplicate names and order", testCandidateScopeAndOrder);
        runTest("exact matching", testExactMatching);
        runTest("input preservation and result copies", testInputPreservationAndResultCopies);
        std::cout << "[PASS] test_filter: 10 test groups\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
