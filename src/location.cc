#include "location.h"

#include <iostream>
#include <list>
#include <string>

using namespace std;

string Location::ToString() const {
  return fileName + ":" + std::to_string(line) + ":" + std::to_string(column);
}

void Location::Reset() {
  fileName = "";
  line = 1;
  column = 1;
}

bool Location::operator<=(const Location& rhs) const {
  cout << ToString() << " <= " << rhs.ToString() << endl;
  return (fileName == rhs.fileName &&
            (line < rhs.line || 
            (line == rhs.line && column <= rhs.column)));
}

bool Range::InRange(const Location& where) const {
  return (begin <= where && where <= end);
}

bool Range::IsSubrange(const Range& range) const {
  return begin <= range.begin && range.end <= end;
}

std::list<size_t> Range::Lines() const {
  std::list<size_t> lines;
  if (Valid()) {
    for (size_t l = begin.line; l <= end.line; ++l) lines.push_back(l);
  }
  return lines;
}

string Range::ToString() const {
  return "{begin: " + begin.ToString() + ", end: " + end.ToString() + "}";
}

bool Range::Valid() const {
  return begin.fileName == end.fileName;
}

bool Range::operator<(const Range& rhs) const {
  return (begin.fileName < rhs.begin.fileName ||
          begin.line < rhs.begin.line ||
          begin.column < rhs.begin.column);
}
