#pragma once
#include "structures/BasicTypes.hpp"

#include "model/Hospital.h"
#include "search/SearchEngine.h"



using HospitalIndexList = SearchArray<size_t>;

struct DuplicateRecord {
    Text id;
    Text name;
    size_t keptIndex;
};

class HospitalStore {
public:
    HospitalStore() = default;

    bool add(const Hospital& hospital);

    size_t addAll(const SearchArray<Hospital>& hospitals);

    const Hospital* findById(const Text& id) const;

    HospitalIndexList findByName(const Text& name) const;

    SearchArray<Hospital> toHospitals(const HospitalIndexList& indexes) const;

    size_t size() const { return hospitals_.size(); }
    bool empty() const { return hospitals_.empty(); }

    const SearchArray<Hospital>& hospitals() const { return hospitals_; }
    const HospitalIdIndex& idIndex() const { return byId_; }
    const HospitalNameIndex& nameIndex() const { return byName_; }
    const SearchArray<DuplicateRecord>& duplicates() const { return duplicates_; }

    SearchEngine makeSearchEngine() const;

private:
    SearchArray<Hospital> hospitals_;
    HospitalIdIndex byId_;
    HospitalNameIndex byName_;
    SearchArray<DuplicateRecord> duplicates_;
};
