#include "search/HospitalFilter.h"

#include "model/Hospital.h"

std::vector<Hospital> HospitalFilter::apply(
    const std::vector<Hospital>& candidates,
    const FilterConditions& conditions
) {
    std::vector<Hospital> result;
    for (const Hospital& hospital : candidates) {
        if (!conditions.district.empty() &&
            hospital.district != conditions.district) {
            continue;
        }
        if (!conditions.hospitalType.empty() &&
            hospital.hospitalType != conditions.hospitalType) {
            continue;
        }
        if (!conditions.emergencyType.empty() &&
            hospital.emergencyType != conditions.emergencyType) {
            continue;
        }
        result.push_back(hospital);
    }
    return result;
}
