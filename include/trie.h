//trie.h
#pragma once

#include <iostream>
#include <istream>
#include <list>
#include <memory>
#include <string>

/*
 * String searching trie used by parser
 */

class TrieNode;
class Trie {
 public:
  Trie();

  void Insert(const std::list<std::string>& tokens);
  void Insert(const std::string& branch);

  std::string GetToken();

  void Reset();

  std::string ReadNext(std::istream& is);
  
  void Dump();

  ~Trie();

 private:
  std::unique_ptr<TrieNode> root_;
  TrieNode* current_;
};

