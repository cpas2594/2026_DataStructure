#pragma once

// 역할: 콘솔 메뉴, 사용자 입력, 결과 출력 기능의 선언.
// TODO: 메뉴 실행, SearchQuery 생성, 검색 결과/오류 출력 인터페이스 선언.
#pragma once

// 역할: 콘솔 메뉴, 사용자 입력, 결과 출력
// 흐름 (발표자료 5쪽): 검색어·조건 입력 -> SearchService -> 목록 출력
//                     -> 번호 선택 시 상세 출력 -> 다시 검색?
// 검색·필터·정렬 알고리즘은 이 파일에 넣지 않음 (SearchService가 담당)

#include <cstddef>
#include <functional>
#include <iosfwd>
#include <string>

#include "data/HospitalStore.h"
#include "model/SearchQuery.h"
#include "search/SearchService.h"
#include "structures/DynamicArray.hpp"

class ConsoleUI {
 public:
  // 한 줄을 읽어 line에 담고 성공 여부를 돌려주는 함수 (입력이 끝나면 false)
  // Windows 콘솔의 한글 입력처럼 읽는 방법을 바꿔야 할 때 main에서 넘겨줌
  using LineReader = std::function<bool(std::string& line)>;

  // reader를 주지 않으면 in에서 std::getline으로 읽음
  ConsoleUI(const SearchService& service, const HospitalStore& store,
            std::istream& in, std::ostream& out, LineReader reader = nullptr);

  // 사용자가 종료하거나 입력이 끝날 때까지 검색을 반복
  void run();

 private:
  const SearchService& service_;
  std::ostream& out_;
  LineReader reader_;
  std::size_t hospitalCount_;

  // 선택지: 실제 데이터에 있는 값만 가나다순으로
  DynamicArray<std::string> districts_;
  DynamicArray<std::string> hospitalTypes_;
  DynamicArray<std::string> emergencyTypes_;

  bool readLine(const char* prompt, std::string& line);

  // false: 입력이 끝났거나 사용자가 종료를 선택함
  bool askQuery(SearchQuery& query);
  bool askOption(const char* title, const DynamicArray<std::string>& options,
                 std::string& selected);
  bool askDetails(const DynamicArray<Hospital>& results);
  bool askSearchAgain();

  void printWelcome(std::size_t hospitalCount) const;
  void printQuerySummary(const SearchQuery& query,
                         const SearchOutcome& outcome) const;
};