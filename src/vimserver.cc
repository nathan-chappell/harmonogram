#include "vimserver.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <list>

using namespace std;

namespace vimserverNames {

const string kColorScheme = ":colorscheme desert";
const string kEnter = "<Enter>";
const string kReverseHighlightGroup = ":highlight reverse \
                                  gui=reverse cterm=reverse term=reverse";
const string kRegL = "\"l\", \":lnext\", \"l\")";
const string kRegH = "\"h\", \":lprev\", \"l\")";
const string kSet = ":set ";
const string kSetReg = ":call setreg(";
const string kWindowSize = "lines=20 columns=85";

size_t VimServer::fileId = 0;

template<typename T>
list<string> MakeStringList(const list<T>& tList) {
  list<string> stringList;
  for (const auto& tItem : tList) stringList.push_back(move(to_string(tItem)));
  return stringList;
}

//joins a string list
string JoinList(const list<string>& stringList, char joiner = ' ') {
  string str;
  for (const auto& s : stringList) str += s + joiner;
  if (str.size() > 0) str.pop_back();
  return str;
}

/*
 * These are a bunch of string functions to simplify the creation of commands to
 * be communicated at the server
 */
string ExprPrefix(const string& serverName) {
  return "gvim --servername " + serverName + " --remote-expr ";
}

string SendPrefix(const string& serverName) {
  return "gvim --servername " + serverName + " --remote-send ";
}

string QuoteDelim(char delim) { return (string)"\"" + delim + "\""; }
string SingleQuote(const string& str) { return "'" + str + "'"; }

string GetColCommand(const string& serverName, char delim = '>') {
  return ExprPrefix(serverName) + 
    SingleQuote("col(\".\") . " + QuoteDelim(delim));
}

string GetExitCommand(const string& serverName, char delim = '>') {
  return SendPrefix(serverName) + 
    SingleQuote(":xa" + kEnter);
}

string GetEditFileCommand(const string& serverName, const string& fileName,
    char delim = '>') {
  return SendPrefix(serverName) + 
    SingleQuote(":edit " + fileName + kEnter);
}

string GetCursorCommand(const string& serverName, const Location& location,
    char delim = '>') {
  return ExprPrefix(serverName) + 
    SingleQuote("cursor(" + to_string(location.line) + ", " + 
        to_string(location.column) + ")");
}

string GetFileNameCommand(const string& serverName, char delim = '>') {
  return ExprPrefix(serverName) + 
    SingleQuote("getreg(\"%\") . " + QuoteDelim(delim));
}

string GetHighlightPatternCommand(const string& serverName, 
    const string& pattern, char delim = '>') {
  return ExprPrefix(serverName) + 
    SingleQuote("matchadd(\"reverse\", \"" + 
        pattern + "\") . " + QuoteDelim(delim));
}

string GetHighlightRangeCommand(const string& serverName, 
    const Range& range, char delim = '>') {
  string rangeString = "[" + JoinList(MakeStringList(range.Lines()), ',') + "]";
  //cout << "rangestring: " << rangeString << endl;
  return ExprPrefix(serverName) + 
    SingleQuote("matchaddpos(\"reverse\"," + 
        rangeString + ") . " + QuoteDelim(delim));
}

string GetLineCommand(const string& serverName, char delim = '>') {
  return ExprPrefix(serverName) + 
    SingleQuote("line(\".\") . " + QuoteDelim(delim));
}

string GetLocListCommand(const string& serverName, const string& locationList,
    char delim = '>') {
  return ExprPrefix(serverName) + 
    SingleQuote("setloclist(0," + locationList + ") . " + QuoteDelim(delim));
}

string GetModeCommand(const string& serverName, char delim = '>') {
  return ExprPrefix(serverName) 
    + SingleQuote("mode(\".\") . " + QuoteDelim(delim));
}

string GetRedrawCommand(const string& serverName, char delim = '>') {
  return SendPrefix(serverName) 
    + SingleQuote(":redraw<Enter>");
}

string GetSetNormalCommand(const string& serverName, char delim = '>') {
  return SendPrefix(serverName) 
    + SingleQuote("<C-\\><C-N>");
}

string GetUnHighlightCommand(const string& serverName, 
    size_t id, char delim = '>') {
  return ExprPrefix(serverName) + 
    SingleQuote((string)"matchdelete(" + to_string(id) + ")");
}

//here is what I set whenever I open vim.  It may be intrusive to some...
const list<string> kDefaultSettings {
  "cindent",
  "et", "ts=2", "sts=2", "sw=2",
  "hidden",
  "hlsearch",
  "mouse=a",
  "nowrap",
  "number",
  "switchbuf=useopen,usetab",
  "textwidth=80"
};

string GetInitialOptionsCommand(const string& serverName, char delim = '>') {
  return SendPrefix(serverName) + SingleQuote(
          kColorScheme + kEnter + 
          kSet + JoinList(kDefaultSettings) + kEnter + 
          kReverseHighlightGroup + kEnter +
          kSetReg + kRegL + kEnter + 
          kSetReg + kRegH + kEnter +
          kSet + kWindowSize + kEnter);
}

//

/*
 * Poll the file at $fileName until the delimiter is found.  The intention is
 * that the existence of the delimiter implies that the vimserver has fully
 * communicated its response, but there is a timeout just in case.
 */
bool GotResponse(const string& fileName, string& response, 
    char delim, double timeout) {
  cout << __func__ << endl;
  double timeDelta = timeout / 20;
  ifstream file(fileName);
  //cout << "response file: " << fileName << endl;

  for (double wait = 0; wait < timeout; wait += timeDelta) {
    getline(file, response, delim);
    if (file.eof()) {
      file.seekg(0);
      cout << delim << " wasn't found in: " << response << endl;
      MySystemCall("sleep .1s");
      continue;
    } else {
      cout << "response: " << response << endl;
      return true;
    }
  }
  return false;
}

string GetServerListCommand() {
  return "echo `gvim --serverlist` \">\" ";
}

/*
 * Return a list of active servers
 */
list<string> ServerList(double timeout) {
  list<string> serverList;
  GetValueFromVimServer<list<string>>(GetServerListCommand(), serverList, '>', timeout);
  cout << "serverList: " << endl;
  for (const auto& s : serverList) cout << s << endl;
  return serverList;
}

VimServer::VimServer(const string& name) : name_(name) {}

bool VimServer::Activate() {
  cout << __func__ << endl;
  active_ = true;
  setNormal = false;
  return RefreshServer(1);
}

bool CaseInsensitiveEqual(const string l, const string r) {
  if (l.size() != r.size()) return false;
  size_t max = l.size();
  for (size_t i = 0; i < max; ++i) {
    if (tolower(l[i]) != tolower(r[i])) return false;
  }
  return true;
}

bool VimServer::CheckServer(double timeout) {
  cout << __func__ << ", " << name_ << endl;
  for (const auto& server : ServerList(timeout)) {
    cout << "checking: " << server << endl;
    if (CaseInsensitiveEqual(name_,server)) {
      cout << name_ << " active!" << endl;
      active_ = true;
      return true;
    }
  }
  cout << name_ << " not active!" << endl;
  active_ = false;
  return false;
}

bool VimServer::Exit() {
  if (SetNormalMode()) {
    MySystemCall(GetExitCommand(GetName()).c_str());
    active_ = false;
    return true;
  } else {
    return false;
  }
}

bool VimServer::GetCursor(Location& location) {
  if (GetValueFromVimServer<size_t>(
          GetLineCommand(name_), location.line) &&

      GetValueFromVimServer<size_t>(
          GetColCommand(name_), location.column) &&

      GetValueFromVimServer<string>(
          GetFileNameCommand(name_), location.fileName)
      ) return true;
  return false;
}

bool VimServer::HighlightPattern(const string& pattern) {
  if (highlightIdMap.count(pattern)) return true;
  HighlighId highlightId;
  if (GetValueFromVimServer<HighlighId>(
      GetHighlightPatternCommand(name_, pattern), highlightId)) {
    highlightIdMap[pattern] = highlightId;
    Redraw();
    return true;
  } 
  return false;
}

bool VimServer::HighlightRange(const Range& range) {
  if (highlightIdMap.count(range.ToString())) return true;
  HighlighId highlightId;
  if (GetValueFromVimServer<HighlighId>(
      GetHighlightRangeCommand(name_, range), highlightId)) {
    highlightIdMap[range.ToString()] = highlightId;
    Redraw();
    return true;
  } 
  return false;
}

bool VimServer::Redraw() {
  if (SetNormalMode()) {
    MySystemCall(GetRedrawCommand(name_));
    return true;
  }
  return false;
}

bool VimServer::RefreshServer(double timeout) {
  if (!active_) return false;
  cout << __func__ << endl;
  //see if anything needs to be done...

  //start up the server...
  //char c;
  //cout << "press y to open new server" << endl;
  //cin >> c;
  //if (c != 'y') return false;
  string command = "gvim --servername " + name_ + " " 
    + JoinList(fileNameList_);
  MySystemCall(command.c_str());

  //wait for it...
  MySystemCall("sleep 1s");

  //set some default options
  cout << "initial options: " << GetInitialOptionsCommand(name_) << endl;
  MySystemCall(GetInitialOptionsCommand(name_).c_str());

  //check to make sure it is freakin open now...
  return (CheckServer());
}

bool VimServer::SetCursor(const Location& location) {
  if (SetNormalMode()) {
    MySystemCall(GetEditFileCommand(name_, location.fileName).c_str());
    MySystemCall(GetCursorCommand(name_, location).c_str());
    return true;
  }
  return false;
}

void VimServer::SetFileNameList(const list<string>& fileNameList) {
  fileNameList_ = fileNameList;
}

bool VimServer::UnHighlightPattern(const string& pattern) {
  if (!highlightIdMap.count(pattern)) return true;
  MySystemCall(GetUnHighlightCommand(name_, highlightIdMap[pattern]).c_str());
  highlightIdMap.erase(pattern);
  Redraw();
  return true;
}

bool VimServer::UnHighlightRange(const Range& range) {
  string rangeString = range.ToString();
  if (!highlightIdMap.count(rangeString)) return true;
  MySystemCall(GetUnHighlightCommand(name_, highlightIdMap[rangeString]).c_str());
  highlightIdMap.erase(rangeString);
  Redraw();
  return true;
}

VimServer::~VimServer() { Exit(); }

/*
 * Turns a range into a "dictionary" for creating a location list entry.
 * Vim help:
 * :help dictionaries
 */
string GetDictionary(const Range& range) {
  return "{\"filename\":\"" + range.begin.fileName + "\", \"lnum\":" + 
    to_string(range.begin.line) + ", \"col\":" + to_string(range.begin.column) + "}";
}

/*
 * Gets all dictionaries for ranges which are highlighted
 */
string GetDictionaryList(const map<Range, HighlighId>& highlightIdMap) {
  string DictionaryList = "[";
  for (const auto& p : highlightIdMap) {
    if (p.first.Valid()) DictionaryList += GetDictionary(p.first) + ",";
  }
  DictionaryList.pop_back();
  return DictionaryList + "]";
}

const string kModeVimEx = "cv";
const string kModeNormalEx = "ce";
const string kModeEnterPrompt = "r";
const string kModeMorePrompt = "rm";
const string kModeConfirm = "r?";
const string kModeShell = "!";

bool CanSwitchToNormalFrom(const string& curMode) {
  return (curMode != kModeVimEx &&
          curMode != kModeNormalEx &&
          curMode != kModeEnterPrompt &&
          curMode != kModeMorePrompt &&
          curMode != kModeConfirm &&
          curMode != kModeShell);
}

bool VimServer::SetNormalMode() {
  if (setNormal) return true;
  string curMode;
  GetValueFromVimServer<string>(GetModeCommand(name_), curMode);
  if (CanSwitchToNormalFrom(curMode)) {
    MySystemCall(GetSetNormalCommand(name_).c_str());
    return true;
  }
  return false;
}

void MySystemCall(const string& command) {
  static size_t count = 0;
  cout << "system call: " << ++count << '\t' << command << endl;
  system(command.c_str());
}

template<>
bool GetValueFromVimServer<list<string>>(const string& command, list<string>& out,
    char delim, double timeout) {
  string response;
  string responseFileName(".response_" + 
      std::to_string(++VimServer::fileId));

  string myCommand = command + " > " + responseFileName;
  std::cout << "STRING LIST Executing: \n" << myCommand << std::endl;
  MySystemCall(myCommand.c_str());

  if (GotResponse(responseFileName, response, delim, timeout)) {
    cout << "got response!" << response << endl;
    std::istringstream iss(response);
    string tmp;
    while (iss.good()) {
      iss >> tmp;
      cout << "read:" << tmp << endl;
      out.push_back(move(tmp));
    }
    //MySystemCall(string("rm " + responseFileName).c_str());
    return true;
  } else {
    std::cout << "No Response!" << std::endl;
    return false;
  }
}

}; //namespace vimserverNames


/*
using namespace vimserverNames;
int main() {
  Location l1{"servertesting",5,2};
  Location l2{"servertesting",7,2};
  Range r1{l1, l2};

  VimServer server("FOO");
  server.SetFileNameList({"servertesting"});
  cout << "activate: " << server.Activate() << endl;
  cout << "server list:\n" << JoinList(ServerList()) << endl;
  cout << "HighlightRange: " << server.HighlightRange(r1) << endl;
  char c;
  cout << "press any key to set cursor" << endl;
  cin >> c;
  server.SetCursor(r1.begin);
  cout << "press any key to unhighlight" << endl;
  cin >> c;
  server.UnHighlightRange(r1);
  cout << "press any key to exit" << endl;
  cin >> c;
  server.Exit();
}
*/
