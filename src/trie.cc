#include "trie.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <iostream>
#include <istream>
#include <list>
#include <string>

using namespace std;

class TrieNode {
 public:
  TrieNode() : c_(0xFF), parent_(nullptr), fallback_(this),
      isToken(false), id_(++id) {}

  TrieNode(char c, TrieNode* fallback, TrieNode* parent) : 
      c_(tolower(c)), parent_(parent), fallback_(fallback),
      isToken(false), id_(++id) {}

  friend Trie;

  friend void TrieDump(TrieNode* subTree,
            string(TrieNode::*visitor)(void)const,
            string(TrieNode::*childVisitor)(void)const);

 private:
  bool Match(char c) { return tolower(c) == c_; }
  bool IsToken() { return isToken; }
  bool IsRoot() {
    return c_ == 0xFF || fallback_ == this || parent_ == nullptr;
  }

  TrieNode* GetNext(char c) {
    for (auto& node : childList_) {
      if (node.Match(c)) return &node;
    }
    if (!IsRoot()) return fallback_->GetNext(c);
    assert(IsRoot());
    return this;
  }

  string Prefix() const {
    const TrieNode* cur = this;
    string prefix;
    while (cur->parent_) {
      prefix.push_back(cur->c_);
      cur = cur->parent_;
    }
    reverse(prefix.begin(), prefix.end());
    return prefix;
  }

  string DebugString() const {
    return "{" + to_string(id_) + " c: " + c_ + 
        ", token? " + to_string(isToken) + "}";
  }

 private:
	char c_;
  TrieNode* parent_;
  TrieNode* fallback_;
  bool isToken;
  size_t id_;
  list<TrieNode> childList_;

  static size_t id;
};

size_t TrieNode::id = 0;

Trie::Trie() : root_(make_unique<TrieNode>()) {
  Reset();
}

void Trie::Insert(const list<string>& tokens) {
  //cout << "inserting list: " << endl;
  Reset();
  for (const auto& token : tokens) {
    Insert(token);
  }
}

void Trie::Insert(const string& branch) {
  //cout << "inserting: " << branch << endl;
  TrieNode* cur = root_.get();
  for (auto c : branch) {
   // Conducted to maintain order based on char
    list<TrieNode>::iterator nextIt = 
      find_if(cur->childList_.begin(), cur->childList_.end(), 
        [c](const TrieNode& node) -> bool { return node.c_ >= c; }
      );
    //if not found, make a new one
    if (nextIt == cur->childList_.end() || nextIt->c_ > c) {
      cur = &*cur->childList_.emplace(nextIt, c, cur->fallback_->GetNext(c), cur);
    }
    else {
      cur = &*nextIt;
    }
  }
  cur->isToken = true;
}

string Trie::GetToken() {
  return current_->Prefix();
}

void Trie::Reset() {
  current_ = root_.get();
}

string Trie::ReadNext(istream& is) {
  string read;
  assert(current_);
  do {
    read.push_back(is.get());
    current_ = current_->GetNext(read.back());
    if (!current_) Reset();
  } while (is.good() && !current_->IsToken());
  return read;
}

void TrieDump(TrieNode* subTree,
    string(TrieNode::*visitor)(void)const,
    string(TrieNode::*childVisitor)(void)const) {
  list<const TrieNode*> Q{subTree};
  while (!Q.empty()) {
    const TrieNode* cur = Q.front();
    cout << "cur: " << (cur->*visitor)() << endl;
    for (const auto& child : cur->childList_) {
      cout << '\t' << '\t' << (child.*childVisitor)() << endl;
      Q.push_back(&child);
    }
    Q.pop_front();
  }
}

void Trie::Dump() {
  TrieDump(root_.get(), &TrieNode::DebugString, &TrieNode::Prefix);
}

Trie::~Trie() = default;

/*
int main() {
  list<string> tokens{"dog", "do", "does", "darn", "fool"};
  Trie trie(tokens);
  trie.Dump();
}
*/
