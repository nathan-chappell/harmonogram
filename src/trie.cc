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
  TrieNode() :
    c_       (0xFF),
    parent_  (nullptr),
    fallback_(this),
    isToken  (false),
    id_      (++id) {}

  TrieNode(char c, TrieNode* fallback, TrieNode* parent) : 
      c_       (tolower(c)),
      parent_  (parent),
      fallback_(fallback),
      isToken  (false),
      id_      (++id) {}

  friend Trie;

  /*
   * This function is designed to work with the member functions
   * associated to the TrieNodes, however this was mostly just playing
   * around and not particularly helpful
   */
  friend void TrieDump(
      TrieNode* subTree,
      string(TrieNode::*visitor)(void)const,
      string(TrieNode::*childVisitor)(void)const);

 private:
  bool Match(char c) const { return tolower(c) == c_; }
  bool IsToken()     const { return isToken; }
  bool IsRoot()      const { return parent_ == nullptr; }

  TrieNode* GetNext(char c)
  {
    for (auto& node : childList_)
    {
      if (node.Match(c)) return &node;
    }
    if (!IsRoot()) return fallback_->GetNext(c);
    assert(IsRoot());
    return this;
  }

  string Prefix() const 
  {
    const TrieNode* cur = this;
    string prefix;
    while (cur->parent_)
    {
      prefix.push_back(cur->c_);
      cur = cur->parent_;
    }
    reverse(prefix.begin(), prefix.end());
    return prefix;
  }

  string DebugString() const 
  {
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


/*
 * Trie::Trie()
 */
Trie::Trie() : root_(make_unique<TrieNode>())
{
  Reset();
}
/*
 * Trie::Insert
 */
void Trie::Insert(const list<string>& tokens)
{
  Reset();
  for (const auto& token : tokens) Insert(token);
}
/*
 * Trie::Insert
 */
void Trie::Insert(const string& branch)
{
  TrieNode* cur = root_.get();
  for (auto c : branch)
  {
    list<TrieNode>::iterator nextIt = 
      find_if(cur->childList_.begin(), cur->childList_.end(), 
          [c](const TrieNode& node) -> bool { return node.c_ >= c; }
          );
    if (nextIt == cur->childList_.end() || nextIt->c_ > c)
    {
      cur = &*cur->childList_.emplace(nextIt, c, cur->fallback_->GetNext(c), cur);
    }
    else
    {
      cur = &*nextIt;
    }
  }
  cur->isToken = true;
}
/*
 * Trie::GetToken
 */
string Trie::GetToken()
{
  return current_->Prefix();
}
/*
 * Trie::Reset
 */
void Trie::Reset()
{
  current_ = root_.get();
}
/*
 * Trie::ReadNext
 */
string Trie::ReadNext(istream& is)
{
  string read;
  assert(current_);
  do {
    read.push_back(is.get());
    current_ = current_->GetNext(read.back());
    if (!current_) Reset();
  } while (is.good() && !current_->IsToken());
  return read;
}
/*
 * TrieDump
 */
void TrieDump(TrieNode* subTree,
    string(TrieNode::*visitor)(void)const,
    string(TrieNode::*childVisitor)(void)const)
{
  list<const TrieNode*> Q{subTree};
  while (!Q.empty())
  {
    const TrieNode* cur = Q.front();
    cout << "cur: " << (cur->*visitor)() << endl;
    for (const auto& child : cur->childList_)
    {
      cout << '\t' << '\t' << (child.*childVisitor)() << endl;
      Q.push_back(&child);
    }
    Q.pop_front();
  }
}
/*
 * Trie::Dump
 */
void Trie::Dump()
{
  TrieDump(root_.get(), &TrieNode::DebugString, &TrieNode::Prefix);
}

Trie::~Trie() = default;

