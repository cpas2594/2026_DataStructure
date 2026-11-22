#include "ui/ResultPrinter.h"

#include <ostream>
#include <string>

namespace {

const char* const kDivider = "------------------------------------------------------------";

// 비어 있는 값은 "정보 없음"으로 표시
const std::string& orNone(const std::string& value) {
    static const std::string kNone = "정보 없음";
    return value.empty() ? kNone : value;
}

// 한글은 콘솔 폭 계산이 환경마다 달라서 setw 대신 라벨을 고정 문자열로 맞춤
void printField(std::ostream& out, const char* label, const std::string& value) {
    out << "    " << label << " : " << orNone(value) << '\n';
}

}  // namespace

void ResultPrinter::printList(std::ostream& out, const DynamicArray<Hospital>& results) {
    out << kDivider << '\n';
    if (results.empty()) {
        out << "검색 결과가 없습니다. 검색어나 조건을 바꿔 다시 검색해 주세요.\n";
        out << kDivider << '\n';
        return;
    }

    out << "검색 결과 " << results.size() << "건 (병원명 -> HPID 순)\n";
    out << kDivider << '\n';
    for (std::size_t i = 0; i < results.size(); ++i) {
        const Hospital& hospital = results[i];
        out << '[' << (i + 1) << "] " << orNone(hospital.name)
            << " (" << orNone(hospital.id) << ")"
            << " | " << orNone(hospital.district)
            << " | " << orNone(hospital.emergencyType)
            << " | 응급실 " << orNone(hospital.emergencyPhone) << '\n';
    }
    out << kDivider << '\n';
    out << "* 실시간 병상 정보가 아닙니다. 방문 전 응급실 전화로 확인하세요.\n";
}

void ResultPrinter::printDetail(std::ostream& out, const Hospital& hospital, std::size_t number) {
    out << kDivider << '\n';
    out << '[' << number << "] " << orNone(hospital.name) << " (" << orNone(hospital.id) << ")\n";
    printField(out, "자치구       ", hospital.district);
    printField(out, "병원 종류    ", hospital.hospitalType);
    printField(out, "응급기관 종류", hospital.emergencyType);
    printField(out, "주소         ", hospital.address);
    printField(out, "대표 전화    ", hospital.phone);
    printField(out, "응급실 전화  ", hospital.emergencyPhone);
    out << kDivider << '\n';
}