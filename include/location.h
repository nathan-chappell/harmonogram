//location.h
#pragma once

#include <string>
#include <list>

struct Location {
  std::string fileName;
  size_t line;
  size_t column;

  void Reset();
  std::string ToString() const;
  bool operator<=(const Location& rhs) const;
};

class Range {
 public:
  Location begin;
  Location end;

  bool InRange(const Location& where) const;
  bool IsSubrange(const Range& range) const;
  std::list<size_t> Lines() const;
  std::string ToString() const;
  bool Valid() const;
  bool operator<(const Range& rhs) const;
};


