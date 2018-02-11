#include <iostream>
#include <iomanip>

#include "pendulum.h"
#include "pendulum_parser.h"
#include "vimserver.h"

using namespace std;
using namespace pendulumNames;
using namespace vimserverNames;

void WaitForInput(const string& message) {
  static char c;
  cout << message << '\n' << "Enter any key to continue" << endl;
  cin >> c;
}

const string kSrcDir {"examples/"};
const list<string> kSrcFiles {"input", "input2"};
list<string> FileNames() {
  list<string> fileNames;
  for (const auto& src : kSrcFiles) fileNames.push_back(kSrcDir + src);
  return fileNames;
}

list<PendulumPtr> pendulums;
VimServer vimserver("FOO");
bool response;
HarmonogramParser parser;

void HighlightTest() {
  WaitForInput("Highlight Pendulums");
  for (const auto& pPtr : pendulums) {
    cout << "pPtr: " << pPtr->name;
    WaitForInput("HighlightPattern");
    response = vimserver.HighlightPattern(pPtr->name);
    cout << "HighlightPattern: " << response << endl;
  }
}

void UnHighlightTest() {
  WaitForInput("UnHighlight Pendulums");
  for (const auto& pPtr : pendulums) {
    cout << "pPtr: " << pPtr->name;
    WaitForInput("UnHighlightPattern");
    response = vimserver.UnHighlightPattern(pPtr->name);
    cout << "UnHighlightPattern: " << response << endl;
  }
}

void SetCursorTest() {
  WaitForInput("Set Cursor");
  for (const auto& p : parser.locationMap) {
    WaitForInput("SetCursor: " + p.second.begin.ToString());
    response = vimserver.SetCursor(p.second.begin);
    cout << "SetCursor: " << response << endl;
  }
}

void Exit() {
  WaitForInput("Exit"); response = vimserver.Exit(); 
  cout << "Exit: " << response << endl;
}

void CheckServer() {
  WaitForInput("CheckServer"); response = vimserver.CheckServer(); 
  cout << "CheckServer: " << response << endl;
}

void Activate() {
  WaitForInput("Activate"); response = vimserver.Activate(); 
  cout << "Activate: " << response << endl;
}

int main() {
  list<string> fileNames = FileNames();
  vimserver.SetFileNameList(fileNames);
  pendulums = parser.Parse(fileNames);
  //for (auto&& pPtr : pendulums) cout << pPtr->ToString() << endl;

  
  for (auto p : parser.locationMap) cout << p.first << 
    ", range: " << p.second.ToString() << endl;

  cout << boolalpha;
  Activate();
  Activate();
  CheckServer();
  Exit();
  CheckServer();

  HighlightTest();
  Exit();
  UnHighlightTest();
  Exit();
  SetCursorTest();

 
}
