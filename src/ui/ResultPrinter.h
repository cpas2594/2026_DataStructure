#pragma once

// 사용 순서 (메인화면에서 호출)
//   1. ResultPrinter::printList(std::cout, results);
//   2. 사용자에게 번호(1 ~ results.size()) 입력받기
//   3. ResultPrinter::printDetail(std::cout, results[번호 - 1], 번호);

#include "model/Hospital.h"
#include "structures/DynamicArray.hpp"

#include <cstddef>
#include <iosfwd>

class ResultPrinter {
public:
    // 결과 건수와 병원별 한 줄 요약(번호, 병원명, HPID, 자치구, 응급기관 종류, 응급실 전화) 출력
    // 결과가 없으면 안내 문구만 출력
    static void printList(std::ostream& out, const DynamicArray<Hospital>& results);

    // 병원 한 곳의 상세 정보 출력. number는 목록에서의 번호(1부터)
    static void printDetail(std::ostream& out, const Hospital& hospital, std::size_t number);
};