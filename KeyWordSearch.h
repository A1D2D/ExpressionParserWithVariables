#pragma once

#include <unordered_map>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <queue>
#include <cstdlib>
#include <sstream>
#include <malloc.h>

class KeyWordSearch {
 private:
   struct Node {
      std::unordered_map<char, Node*> children;
      std::vector<size_t> keywordIds;
      Node* failure = nullptr;
      Node() {}
   };

   Node* root = nullptr;

 public:
   bool machCase = true;
   bool firstElement = false;

   struct FoundKeyWord {
      size_t position;
      size_t length;
      size_t id;
   };

   struct SearchKeyWord {
      std::string str;
   };

   KeyWordSearch() {
      
   }

   ~KeyWordSearch() {
      deleteTrie();
   }

   void addKeyWord(std::string keyword) {
      SearchKeyWord key;
      key.str = keyword;
      if(!machCase) std::transform(key.str.begin(), key.str.end(), key.str.begin(), [](unsigned char c){ return std::tolower(c); });
      keyWords.push_back(key.str);
   }

   Node* buildTrie() {
      Node* root = new Node();
      for (size_t i = 0; i < keyWords.size(); ++i) {
         Node* curr = root;
         
         for (char c : keyWords[i]) {
            if (!curr->children.count(c)) {
               curr->children[c] = new Node();
            }
            curr = curr->children[c];
         }
         curr->keywordIds.push_back(i);
      }

      std::queue<Node*> q;
      for (auto& [c, child] : root->children) {
         child->failure = root;
         q.push(child);
      }

      while (!q.empty()) {
         Node* curr = q.front();
         q.pop();
         for (auto& [c, child] : curr->children) {
            Node* failureNode = curr->failure;
            while (failureNode != nullptr && !failureNode->children.count(c)) {
               failureNode = failureNode->failure;
            }
            if (failureNode == nullptr) {
               child->failure = root;
            } else {
               child->failure = failureNode->children[c];
               child->keywordIds.insert(child->keywordIds.end(), child->failure->keywordIds.begin(), child->failure->keywordIds.end());
            }
            q.push(child);
         }
      }

      return root;
   }

   void deleteTrie() {
      if(!root) return;

      std::queue<Node*> queue;

      for (auto& [_, child] : root->children) {
         queue.push(child);
      }
      root->children.clear();

      Node* current;
      while (!queue.empty()) {
         current = queue.front();
         queue.pop();

         for (auto& [_, child] : current->children) {
            queue.push(child);
         }

         delete current;
      }
      delete root;
      root = nullptr;
   }


   static std::vector<std::string> split(const std::string& str, char delimiter) {
      std::vector<std::string> strings;
      std::string::size_type start = 0, end = 0;

      while ((end = str.find(delimiter, start)) != std::string::npos) {
         strings.push_back(str.substr(start, end - start));
         start = end + 1;
      }

      strings.push_back(str.substr(start));

      return strings;
   }

   std::vector<FoundKeyWord> searchForKeywords(std::string text) {
      if(!machCase) std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c){ return std::tolower(c); });

      std::vector<FoundKeyWord> result;
      if (text.empty() || keyWords.empty()) return result;

      if(!root) root = buildTrie();

      // Traverse the text and find keywords
      bool isFirst = true;
      bool record = false;
      size_t n = text.length();
      Node* curr = root;
      for (size_t i = 0; i < n; ++i) {
         char c = text[i];

         while (curr != root && !curr->children.count(c)) {
            curr = curr->failure;
            record = false;
         }
         if (curr->children.count(c)) {
            curr = curr->children[c];
            if(isFirst) record = true;
         }
         if ((record || !firstElement) && !curr->keywordIds.empty()) {
            for (auto id : curr->keywordIds) {
               result.push_back({i - keyWords[id].length() + 1, keyWords[id].length(), id});
            }
         }

         if(c != ' ') isFirst = false;
         if(c == '\n') {
            isFirst = true;
            record = false;
         }
      }

      return result;
   }

   std::vector<std::string> keyWords;
};