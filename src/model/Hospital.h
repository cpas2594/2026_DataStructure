#pragma once
#include "structures/BasicTypes.hpp"


struct Hospital {
    Text id;
    Text name;
    Text district;
    Text hospitalType;
    Text emergencyType;
    Text address;
    Text phone;
    Text emergencyPhone;
    bool emergencyRoomOpen = false;
};
