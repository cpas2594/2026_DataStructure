#pragma once

#include "model/Hospital.h"               
#include "structures/HashTable.hpp"       
#include <cstddef>
#include <string>
#include <vector>

using HospitalIdIndex = HashTable<std::size_t>;
using HospitalNameIndex = HashTable<std::vector<std::string>>;

class SearchEngine {
public:
    SearchEngine(const std::vector<Hospital>& hospitals,
                 const HospitalIdIndex& byId,
                 const HospitalNameIndex& byName);

    std::vector<Hospital> search(const std::string& input) const;

    static std::string trim(const std::string& input);

private:
    const std::vector<Hospital>& hospitals_;
    const HospitalIdIndex& byId_;
    const HospitalNameIndex& byName_;
};
#pragma once
