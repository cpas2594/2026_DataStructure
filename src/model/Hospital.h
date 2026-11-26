#pragma once

#include <string>

struct Hospital {
    std::string id;
    std::string name;
    std::string district;
    std::string hospitalType;
    std::string emergencyType;
    std::string address;
    std::string phone;
    std::string emergencyPhone;
    bool emergencyRoomOpen = false;
};
