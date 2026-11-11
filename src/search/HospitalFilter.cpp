#include "search/HospitalFilter.h"

#include "model/Hospital.h"

std::vector<Hospital> HospitalFilter::apply(
    const std::vector<Hospital>& candidates,
    const FilterConditions& conditions
) {
    std::vector<Hospital> result;
    for (const Hospital& hospital : candidates) {
        // 미선택 조건은 통과시키고, 선택된 조건은 모두 일치해야 합니다.
        if ((conditions.district.empty() ||
             hospital.district == conditions.district) &&
            (conditions.hospitalType.empty() ||
             hospital.hospitalType == conditions.hospitalType) &&
            (conditions.emergencyType.empty() ||
             hospital.emergencyType == conditions.emergencyType)) {
            result.push_back(hospital);
        }
    }
    return result;
}
