#include "pendulum_parser.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <climits>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <list>
#include <string>
#include <utility>

using namespace pendulumNames;

//Token stuff
enum TokenName {
  kTokAmplitude,
  kTokCenter,
  kTokColor,
  kTokComma,
  kTokCommentBegin,
  kTokCommentEnd,
  kTokCycles,
  kTokDirection,
  kTokFrequency,
  kTokInvalid,
  kTokName,
  kTokOscillation,
  kTokPendulumBegin,
  kTokPendulumEnd,
  kTokRotation,
  kTokStartPhase,
  kTokType
};

const list<pair<TokenName, string>> tokenList {
 {kTokAmplitude, "amplitude:"},
 {kTokCenter, "center:"},
 {kTokColor, "color:"},
 {kTokComma, ","},
 {kTokCommentBegin, "/*"},
 {kTokCommentEnd, "*/"},
 {kTokCycles, "cycles:"},
 {kTokDirection, "direction:"},
 {kTokFrequency, "frequency:"},
 {kTokName, "name:"},
 {kTokOscillation, "oscillation"},
 {kTokPendulumBegin, "{"},
 {kTokPendulumEnd, "}"},
 {kTokRotation, "rotation"},
 {kTokStartPhase, "startphase:"},
 {kTokType, "type:"}
};

list<pair<TokenName, string>> GetTokenList() {
  return {
 {kTokAmplitude, "amplitude:"},
 {kTokCenter, "center:"},
 {kTokColor, "color:"},
 {kTokComma, ","},
 {kTokCommentBegin, "/*"},
 {kTokCommentEnd, "*/"},
 {kTokCycles, "cycles:"},
 {kTokDirection, "direction:"},
 {kTokFrequency, "frequency:"},
 {kTokName, "name:"},
 {kTokOscillation, "oscillation"},
 {kTokPendulumBegin, "{"},
 {kTokPendulumEnd, "}"},
 {kTokRotation, "rotation"},
 {kTokStartPhase, "startphase:"},
 {kTokType, "type:"}
};
}

TokenName StringToTokenName(const string& str) {
  for (const auto& p : tokenList) {
    if (str == p.second) return p.first;
  }
  return kTokInvalid;
}

//
//Parsing Utilities, forward declared Just in Case...

Color ReadColor(HarmonogramParser& parser);
void ReadCommaDouble(HarmonogramParser& parser, double& d);
void ReadComment(HarmonogramParser& parser);
list<PendulumPtr> ReadCurrentInput(HarmonogramParser& parser);
string ReadIdentifier(HarmonogramParser& parser);
PendulumPtr ReadPendulum(HarmonogramParser& parser);
Position ReadPosition(HarmonogramParser& parser);
SimplePendulum::Type ReadType(HarmonogramParser& parser);
//

//From the HarmonogramParser class

HarmonogramParser::HarmonogramParser() {
  location_.Reset();
  for (const auto& p : GetTokenList()) {
    trie_.Insert(p.second);
  }
}

bool HarmonogramParser::Advance() {
  lastRead = move(trie_.ReadNext(GetStream()));
  AdvanceCursor();
  return input_->good();
}

void HarmonogramParser::AdvanceCursor() {
  if (lastRead.find('\n') != lastRead.npos) location_.column = 0;
  location_.line += count_if(lastRead.begin(), lastRead.end(),
      [](char c) -> bool { return c == '\n'; }
    );
  location_.column += lastRead.size() - lastRead.find_last_of('\n');
}

void HarmonogramParser::Error(const string& function, const string& message) {
  cout << function << "(): " << message << " at: " << GetCursor() << endl;
  assert(false);
}

string HarmonogramParser::GetCursor() {
  return location_.ToString();
}

list<PendulumPtr> HarmonogramParser::Parse(const list<string>& fileNameList) {
  locationMap.clear();
  lastRead = "";
  list<PendulumPtr> pendulumPtrList;
  for (const auto& fileName : fileNameList) {
    cout << "parsing: " << fileName << endl;
    location_.Reset();
    location_.fileName = fileName;
    ifstream file(fileName);
    if (!file.good()) {
      cout << "couldn't open file: " << fileName << endl;
      file.close();
      continue;
    }
    input_ = &file;
    //read the pendulums from the file
    pendulumPtrList.splice(pendulumPtrList.end(), 
        move(ReadCurrentInput(*this)));
    file.close();
  }
  return pendulumPtrList;
}

//

Color ReadColor(HarmonogramParser& parser) {
  Color color;
  parser.GetStream() >> color.R;
  ReadCommaDouble(parser, color.G);
  ReadCommaDouble(parser, color.B);
  ReadCommaDouble(parser, color.A);
  return color;
}

void ReadCommaDouble(HarmonogramParser& parser, double& d) {
  parser.Advance();
  switch (StringToTokenName(parser.GetToken())) {
    case kTokComma : parser.GetStream() >> d; break;
    default : parser.Error( __func__, "Expected a ','");
  }
}

void ReadComment(HarmonogramParser& parser) {
  while (parser.Good()) {
    parser.Advance();
    switch (StringToTokenName(parser.GetToken())) {
      case kTokCommentEnd : return; break;
      default : continue;
    }
  }
  parser.Error( __func__, "Expected '*/'");
}

list<PendulumPtr> ReadCurrentInput(HarmonogramParser& parser) {
  unique_ptr<CompoundPendulum> compoundPtr(new CompoundPendulum());
  double& cyclesRef = compoundPtr->cycles_;
  list<PendulumPtr> pendulumPtrList;
  Location startLocation = parser.GetLocation();
  bool read = false;
  Range range;
  while (parser.Good() && !read) {
    parser.Advance();
    Location location = parser.GetLocation();
    switch (StringToTokenName(parser.GetToken())) {
      case kTokPendulumBegin :
        pendulumPtrList.push_back(move(ReadPendulum(parser)));
        if (!pendulumPtrList.back()->IsValid()) {
          parser.Error( __func__, "Invalid Pendulum");
        } 
        if (parser.locationMap.count(pendulumPtrList.back()->name)) {
          cout << "warning, overwriting node: " << 
            pendulumPtrList.back()->name << endl;
        }
        range = Range{move(location), move(parser.GetLocation())};
        cout << pendulumPtrList.back()->name << " -> " << range.ToString() << endl;
        parser.locationMap[pendulumPtrList.back()->name] = range;
        break;
      case kTokCenter : compoundPtr->center = ReadPosition(parser); break;
      case kTokColor : compoundPtr->color = ReadColor(parser); break;
      case kTokCommentBegin : ReadComment(parser); break;
      case kTokCycles: parser.GetStream() >> cyclesRef; break;
      case kTokName : {
                        compoundPtr->name = ReadIdentifier(parser); 
                      } break;
      default : 
        if (parser.Good()) parser.Error( __func__, "Expected '{' or '/*'");
        else read = true;
    }
  }
  for (auto& pendulumPtr : pendulumPtrList) {
    compoundPtr->AddPendulum(pendulumPtr.get());
  }
  compoundPtr->SetPreferredBufferSize();
  parser.locationMap[compoundPtr->name] = Range{startLocation, parser.GetLocation()};
  pendulumPtrList.push_back(move(compoundPtr));
  return pendulumPtrList;
}

string ReadIdentifier(HarmonogramParser& parser) {
  string id;
  parser.GetStream() >> std::ws;
  while (parser.Good() && isalnum(parser.GetStream().peek())) {
      id.push_back(parser.GetStream().get());
  }
  return id;
}

PendulumPtr ReadPendulum(HarmonogramParser& parser) {
  unique_ptr<SimplePendulum> pendulumPtr(new SimplePendulum());
  Location location = parser.GetLocation();
  bool read = false;
  double startPhase;
  while (parser.Good() && !read && 
      StringToTokenName(parser.GetToken()) != kTokPendulumEnd) {
    parser.Advance();
    switch (StringToTokenName(parser.GetToken())) {
      case kTokType : pendulumPtr->type = ReadType(parser); break;
      case kTokName : pendulumPtr->name = ReadIdentifier(parser); 
                      break;
      case kTokCenter : pendulumPtr->center = ReadPosition(parser); break;
      case kTokFrequency : parser.GetStream() >> pendulumPtr->frequency.value;
                           break;
      case kTokColor : pendulumPtr->color = ReadColor(parser); break;
      case kTokStartPhase : parser.GetStream() >> startPhase;
                            pendulumPtr->frequency.SetStartPhase(startPhase);
                            break;
      case kTokDirection : pendulumPtr->direction = ReadPosition(parser); break;
      case kTokAmplitude : parser.GetStream() >> pendulumPtr->amplitude; break;
      case kTokPendulumEnd: read = true; break;
      case kTokCommentBegin : ReadComment(parser); break;
      default :
        cout << parser.GetToken() << endl;
        parser.Error( __func__, "Expected an Attribute");
    }
  }
  pendulumPtr->SetPreferredBufferSize();
  return move(pendulumPtr);
}

Position ReadPosition(HarmonogramParser& parser) {
  Position position;
  parser.GetStream() >> position.x;
  parser.Advance();
  switch (StringToTokenName(parser.GetToken())) {
    case kTokComma : parser.GetStream() >> position.y; break;
    default: parser.Error( __func__, "Expected a ','");
  }
  return position;
}

SimplePendulum::Type ReadType(HarmonogramParser& parser) {
  //parser.Advance();
  string typeName;
  typeName = ReadIdentifier(parser);
  for (auto& c : typeName) c = tolower(c);
  switch (StringToTokenName(typeName)) {
    case kTokOscillation : return SimplePendulum::kOscillation; break;
    case kTokRotation : return SimplePendulum::kRotation; break;
    default : parser.Error( __func__, "Expected a SimplePendulum::Type"); 
              return SimplePendulum::kInvalid;
  }
}

/*
double pendulumNames::PendulumBase::timeDelta = .2;

int main(int argc, char** argv) {
  HarmonogramParser parser;
  parser.Parse({"input", "input2"});
}
*/
