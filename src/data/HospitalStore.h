#pragma once

#include "model/Hospital.h"
#include "search/SearchEngine.h"

#include <cstddef>
#include <string>
#include "structures/DynamicArray.hpp"

using HospitalIndexList = DynamicArray<std::size_t>;

struct DuplicateRecord {
    std::string id;
    std::string name;
    std::size_t keptIndex;
};

class HospitalStore {
public:
    HospitalStore() = default;

    bool add(const Hospital& hospital);

    std::size_t addAll(const DynamicArray<Hospital>& hospitals);

    const Hospital* findById(const std::string& id) const;

    HospitalIndexList findByName(const std::string& name) const;

    DynamicArray<Hospital> toHospitals(const HospitalIndexList& indexes) const;

    std::size_t size() const { return hospitals_.size(); }
    bool empty() const { return hospitals_.empty(); }

    const DynamicArray<Hospital>& hospitals() const { return hospitals_; }
    const HospitalIdIndex& idIndex() const { return byId_; }
    const HospitalNameIndex& nameIndex() const { return byName_; }
    const DynamicArray<DuplicateRecord>& duplicates() const { return duplicates_; }

    SearchEngine makeSearchEngine() const;

private:
    DynamicArray<Hospital> hospitals_;
    HospitalIdIndex byId_;
    HospitalNameIndex byName_;
    DynamicArray<DuplicateRecord> duplicates_;
};
