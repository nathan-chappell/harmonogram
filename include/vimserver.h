//vimserver.h
#pragma once

#include <list>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>

#include "location.h"

namespace vimserverNames {
using std::string;
using std::list;
using std::map;

typedef size_t HighlighId;

//chokepoint for system calls
void MySystemCall(const std::string& command);

class VimServer {
 public:
  static size_t fileId;

  VimServer(const string& name = "VimServer");

  bool Activate();
  bool CheckServer(double timeout = .5);
  bool Exit();
  bool GetCursor(Location& location);
  string GetName() { return name_; };
  bool HighlightPattern(const string& pattern);
  bool HighlightRange(const Range& range);
  bool IsActive() { return active_; }
  bool Redraw();
  bool SetCursor(const Location& location);
  void SetFileNameList(const list<string>&);
  bool SetNormalMode();
  bool UnHighlightPattern(const string& pattern);
  bool UnHighlightRange(const Range& range);
  ~VimServer();

  //TODO... this is just for debugging...
  //friend int main();
  //used to bypass multiple checks of "SetNormalMode()"
  bool setNormal;
 private:
  bool RefreshServer(double timeout = .2);
  list<string> fileNameList_;
  map<string, HighlighId> highlightIdMap;
  string name_;
  bool active_;
};

bool GotResponse(const string& fileName, string& response, 
    char delim, double timeout);

template<typename T>
bool GetValueFromVimServer(const string& command, T& out,
    char delim = '>', double timeout = .1) {
  string response;
  //pseudo-unique (hidden) file name for the response
  string responseFileName(".response_" + 
      std::to_string(++VimServer::fileId));

  //the vim server outputs to stdout, so redirect it to our response file.
  string myCommand = command + " > " + responseFileName;
  MySystemCall(myCommand.c_str());

  if (GotResponse(responseFileName, response, delim, timeout)) {
    //here is the "default" way to convert the string to type T, it could be
    //paramterized, but this implementation will suffice for my purposes
    std::cout << response << std::endl;
    std::istringstream(response) >> out;
    MySystemCall(string("rm " + responseFileName).c_str());
    return true;
  } else {
    //note, no attempt is made to clean up the file...  maybe implement a
    //scheduler to deal with this a little better...
    return false;
  }
}

template<>
bool GetValueFromVimServer<list<string>>(const string& command, list<string>& out,
    char delim, double timeout);

}; //namespace vimserverNames
