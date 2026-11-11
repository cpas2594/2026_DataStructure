#include "model/Hospital.h"
#include "search/HospitalFilter.h"

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
                      const std::string& district) {
    Hospital hospital{};
    hospital.id = id;
    hospital.name = name;
    hospital.district = district;
    return hospital;
}

void testDistrictFilter() {
    // 실제 CSV의 병원 정보와 주소에서 확인한 자치구를 사용합니다.
    // CSV 파싱 및 자치구 추출 자체는 데이터 모듈에서 담당합니다.
    const std::vector<Hospital> candidates = {
        makeHospital("A1100011", "가톨릭대학교여의도성모병원", "영등포구"),
        makeHospital("A1121013", "가톨릭대학교은평성모병원", "은평구"),
        makeHospital("A1100037", "대림성모병원", "영등포구")
    };
    const auto before = candidates;

    const auto selected = HospitalFilter::apply(candidates, {"영등포구"});
    CHECK(selected.size() == 2);
    CHECK(selected[0].id == "A1100011");
    CHECK(selected[1].id == "A1100037");
    CHECK(selected[0].district == "영등포구");
    CHECK(selected[1].district == "영등포구");

    const auto all = HospitalFilter::apply(candidates, {});
    CHECK(all.size() == candidates.size());
    for (std::size_t i = 0; i < candidates.size(); ++i) {
        CHECK(all[i].id == candidates[i].id);
        CHECK(all[i].name == candidates[i].name);
        CHECK(all[i].district == candidates[i].district);
        CHECK(candidates[i].id == before[i].id);
        CHECK(candidates[i].name == before[i].name);
        CHECK(candidates[i].district == before[i].district);
    }

    CHECK(HospitalFilter::apply(candidates, {"없는구"}).empty());
    CHECK(HospitalFilter::apply(candidates, {"영등포"}).empty());
    CHECK(HospitalFilter::apply({}, {"영등포구"}).empty());
    CHECK(HospitalFilter::apply({}, {}).empty());
}
}  // namespace

int main() {
    try {
        testDistrictFilter();
        std::cout << "[PASS] test_filter: district, unselected, no match, empty, order, input preservation\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
