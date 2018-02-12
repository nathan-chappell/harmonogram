//pendulum_parser.h
#pragma once

#include <fstream>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <string>

#include "location.h"
#include "pendulum.h"
#include "trie.h"

namespace pendulumNames {

using std::cout;
using std::endl;
using std::ifstream;
using std::istream;
using std::list;
using std::map;
using std::move;
using std::pair;
using std::string;
using std::unique_ptr;

const string kServerName = "PendulumServer";
typedef string PendulumId;

/*
 * Simple parser using the trie implemented here to read config
 * scripts for the harmonogram program.  The examples should
 * entirely describe the syntax.
 */
class HarmonogramParser {
 public:

  HarmonogramParser();

  list<PendulumPtr> Parse(const list<string>& fileNameList);

  bool     Advance();
  void     Error(const string& function, const string& message);
  string   GetCursor();
  Location GetLocation() { return location_; }
  istream& GetStream()   { return *input_; }
  string   GetToken()    { return trie_.GetToken(); }
  bool     Good()        { return input_->good(); }

  string lastRead;
  map<PendulumId, Range> locationMap;

 private:
  istream* input_;
  Trie     trie_;
  Location location_;

  void AdvanceCursor();
};

}; //namespace pendulumNames
