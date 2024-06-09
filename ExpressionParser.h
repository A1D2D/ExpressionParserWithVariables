#ifndef GIT_CL_EXPRESSION_PARSER_EXPRESSION_PARSER_H
#define GIT_CL_EXPRESSION_PARSER_EXPRESSION_PARSER_H

#include <unordered_map>
#include <vector>
#include <string>
#include <queue>
#include <cmath>
#include <regex>


namespace StringUtils {

   std::string escapeRegex(const std::string& keyword) {
      static const std::regex specialChars{R"([-[\]{}()*+?.,\^$|#\s])"};
      return std::regex_replace(keyword, specialChars, R"(\$&)");
   }

   class KeyWordSearch {
   public:
      struct MatchDetail {
         std::string keyword;
         size_t position;
         size_t id;
      };

      struct SearchKeyword {
         std::string keyWord;
         bool caseSensitive = true;
         bool lineStart = false;
      };

      KeyWordSearch() = default;

      ~KeyWordSearch() = default;

      std::vector<MatchDetail> findKeywords(const std::string& text) const {
         std::vector<MatchDetail> matches;
         std::vector<SearchKeyword> tempKeyWords = keyWords;
         std::vector<size_t> sortedIDs = sortKeywordsByLength(tempKeyWords);
         std::regex pattern(createRegexPattern(tempKeyWords), std::regex::multiline);//2048

         auto wordsBegin = std::sregex_iterator(text.begin(), text.end(), pattern);
         auto wordsEnd = std::sregex_iterator();
         for (std::sregex_iterator it = wordsBegin; it != wordsEnd; ++it) {
            const std::smatch& match = *it;
            for (size_t j = 1; j < match.size(); ++j) {
               if (match[j].matched) {
                  matches.push_back({match.str(j), static_cast<size_t>(match.position(j)), sortedIDs[j-1]});
               }
            }
         }
         return matches;
      }

      std::vector<SearchKeyword> keyWords;

      void addKeyWord(const std::string& keyWord, const bool& caseSensitive, const bool& lineStart) {
         keyWords.push_back({keyWord,caseSensitive,lineStart});
      }

      void removeKeyWord(const std::string &keyWord) {
         keyWords.erase(
            std::remove_if(keyWords.begin(), keyWords.end(),
               [&keyWord](const SearchKeyword& keyword) {
                  return keyword.keyWord == keyWord;
               }),
            keyWords.end()
         );
      }

   private:
      static std::vector<size_t> sortKeywordsByLength(std::vector<SearchKeyword>& keywords) {
         std::vector<std::pair<size_t , SearchKeyword>> pairs(keywords.size());
         for (int i = 0; i < keywords.size(); ++i) {
            pairs[i] = std::make_pair(i, keywords[i]);
         }

         std::sort(pairs.begin(), pairs.end(), [](const std::pair<size_t , SearchKeyword>& a, const std::pair<size_t , SearchKeyword>& b) {
            return a.second.keyWord.size() > b.second.keyWord.size();
         });

         for (int pID = 0; pID < keywords.size(); ++pID) {
            keywords[pID] = pairs[pID].second;
         }

         std::vector<size_t> sortedIDs(keywords.size());
         for (int pID = 0; pID < keywords.size(); ++pID) {
            sortedIDs[pID] = pairs[pID].first;
         }
         return sortedIDs;
      }

      static std::string createRegexPattern(const std::vector<SearchKeyword>& keywords) {
         std::string pattern;
         for (size_t i = 0; i < keywords.size(); ++i) {
            if (i != 0) {
               pattern += "|";
            }
            pattern += "(";
            if (keywords[i].lineStart) {
               pattern += "^[\\t| ]*";
            }

            if (keywords[i].caseSensitive) {
               pattern += escapeRegex(keywords[i].keyWord);
            } else {
               std::string caseless_pattern;
               for (char c : keywords[i].keyWord) {
                  caseless_pattern += "[" + std::string(1, (char)std::toupper(c)) + "|" + std::string(1, (char)std::tolower(c)) + "]";
               }
               pattern += caseless_pattern;
            }
            pattern += ")";
         }
         return pattern;
      }
   };

}

using namespace StringUtils;

template<typename T>
class ExpressionParser {
private:
   enum EXOP_En {
      OP_UNKNOWN = 'u',
      //Arithmetic Operators
      OPA_NEG = 'n',
      OPA_ADD = '+',
      OPA_SUB = '-',
      OPA_MUL = '*',
      OPA_DIV = '/',
      OPA_MOD = '%',
      //Comparison Operators
      OPC_EQUAL = '=',
      OPC_NOTEQ = 'e',
      OPC_LESST = '<',
      OPC_GREET = '>',
      OPC_ELEST = 'l',
      OPC_EGRET = 'g',
      //Logical Operators
      OPL_AND = 'a',
      OPL_OR = 'o',
      OPL_NOT = '!',
      //Bitwise Operators
      OPB_BWAND = '&',
      OPB_BWOR = '|',
      OPB_BWXOR = '^',
      OPB_BWNOT = '~',
      OPB_BWLSH = 'L',
      OPB_BWRSH = 'R'

   };
public:
   enum EXTreeTokenType {
      EXTTT_OP = 0,
      EXTTT_NAMEID = 1,
      EXTTT_NUM = 2
   };

   struct ExOperator {
      EXOP_En op;
      int placeScore = 0;
      int layerScore = 0;

      ExOperator() : op(OP_UNKNOWN), placeScore(0), layerScore(0) {}
   };

   struct EXTreeNode {
      EXTreeTokenType type;
      union {
         struct {
            EXOP_En op;
            EXTreeNode *left;
            EXTreeNode *right;
         };
         size_t valueID;
         T num;
      };
   };

   struct EXTreeToken {
      EXTreeTokenType type;
      union {
         ExOperator exOperator;
         size_t valueID = 0;
         T num;
      };

      EXTreeToken() : type(EXTTT_OP) {
         new(&exOperator) ExOperator();
      }

      ~EXTreeToken() {
         exOperator.~ExOperator();
      }

   };

public:
   ExpressionParser() = default;

   explicit ExpressionParser(const std::string &expression) {
      exprSTR = expression;
   }

   ~ExpressionParser() {
      exprTokens.clear();
      exprSTR = "";
      deleteTree();
   }

   void buildTree(std::string expression, const std::vector<std::string> &inputs) {
      exprTokens = tokenizeExpression(expression, inputs);
      rootNode = buildTree(exprTokens);
   }

   void buildTree(const std::vector<std::string> &inputs) {
      exprTokens = tokenizeExpression(exprSTR, inputs);
      rootNode = buildTree(exprTokens);
   }

   T evaluate(const std::vector<T> &values) {
      return evaluate(rootNode, values);
   }

   void deleteTree() {
      deleteTree(rootNode);
   }

   std::string exprSTR;
   EXTreeNode *rootNode = nullptr;
   std::vector<EXTreeToken> exprTokens;

private:
   bool isDigit(char c) {
      return std::isdigit(static_cast<unsigned char>(c));
   }

   EXOP_En getEXOPEnum(const std::string &ch, int &id,const bool& prevWasOp) {
      char mainCh = ch[id];
      bool secUV = (id <= ch.size() - 1);
      switch (mainCh) {
         case '+':
            return OPA_ADD;
         case '-':
            if(prevWasOp) {
               return OPA_NEG;
            } else return OPA_SUB;
         case '*':
            return OPA_MUL;
         case '/':
            return OPA_DIV;
         case '%':
            return OPA_MOD;
         case '^':
            return OPB_BWXOR;
         case '~':
            return OPB_BWNOT;
         case '=':
            id++;
            return OPC_EQUAL;
         case '!':
            if (secUV) return OPL_NOT;
            if (ch[id + 1] == '=') {
               id++;
               return OPC_NOTEQ;
            } else return OPL_NOT;
         case '<':
            if (secUV) return OPC_LESST;
            if (ch[id + 1] == '=') {
               id++;
               return OPC_ELEST;
            } else if (ch[id + 1] == '<') {
               id++;
               return OPB_BWLSH;
            } else return OPC_LESST;
         case '>':
            if (secUV) return OPC_GREET;
            if (ch[id + 1] == '=') {
               id++;
               return OPC_EGRET;
            } else if (ch[id + 1] == '>') {
               id++;
               return OPB_BWRSH;
            } else return OPC_GREET;
         case '&':
            if (secUV) return OPB_BWAND;
            if (ch[id + 1] == '&') {
               id++;
               return OPL_AND;
            } else return OPB_BWAND;
         case '|':
            if (secUV) return OPB_BWOR;
            if (ch[id + 1] == '|') {
               id++;
               return OPL_OR;
            } else return OPB_BWOR;

         default:
            return OP_UNKNOWN;
      }
   }

   std::vector<EXTreeToken> tokenizeExpression(const std::string &expression, const std::vector<std::string> &inputs) {
      std::vector<EXTreeToken> tokens;
      KeyWordSearch search;
      for (const auto & input : inputs) {
         search.addKeyWord(input, true, false);
      }
      std::vector<KeyWordSearch::MatchDetail> foundKeys = search.findKeywords(expression);

      int OPID = 0;
      int opDepth = 0;
      int nextKey = 0;

      bool prevWasOp = true;

      //numEx
      T snum = 0;
      int snumL = 0;
      int dot = 0;
      bool isNum = false;
      bool hasDot = false;

      for (int sID = 0; sID < expression.size(); sID++) {
         bool jumped = false;
         bool numSkipped = false;
         if (expression[sID] == ' ') {
            jumped = true;
            goto skipNumAdd;
         }
         if (expression[sID] == '(') {
            opDepth++;
            jumped = true;
            prevWasOp = true;
            goto skipNumAdd;
         }
         if (expression[sID] == ')') {
            opDepth--;
            jumped = true;
            prevWasOp = false;
            goto skipNumAdd;
         }
         {
            bool interPrew = foundKeys.size() > nextKey && sID == foundKeys[nextKey].position;
            if (interPrew) {
               sID += static_cast<int>(foundKeys[nextKey].keyword.length() - 1);
               EXTreeToken token;
               token.valueID = foundKeys[nextKey++].id;
               token.type = EXTTT_NAMEID;
               tokens.push_back(token);
               jumped = true;
               numSkipped = true;
               prevWasOp = false;
               goto skipNumAdd;
            }
            EXOP_En expr = getEXOPEnum(expression, sID, prevWasOp);
            if (expr != OP_UNKNOWN) {
               ExOperator exOperator;
               exOperator.layerScore = opDepth;
               exOperator.op = expr;
               exOperator.placeScore = expression.length() - OPID++;

               EXTreeToken token;
               token.exOperator = exOperator;
               token.type = EXTTT_OP;
               tokens.push_back(token);
               jumped = true;
               numSkipped = true;
               prevWasOp = true;
               goto skipNumAdd;
            }
            if (isDigit(expression[sID]) || expression[sID] == '.') {
               if (expression[sID] == '.') {
                  dot = snumL;
                  hasDot = true;
               } else {
                  isNum = true;
                  snum *= 10;
                  snum += (expression[sID] - 48);
                  snumL++;
               }
            }
         }

         skipNumAdd:
         bool numEnd = !(isDigit(expression[sID + 1]) || expression[sID + 1] == '.');
         bool exprEnd = sID >= expression.size() - 1;
         if (isNum && (numEnd || exprEnd || jumped)) {
            T num = snum;
            T div = powf(10, (float)snumL - (float)dot);
            if (hasDot)num /= div;
            snum = 0;
            snumL = 0;
            dot = 0;
            isNum = false;
            hasDot = false;


            EXTreeToken token;
            token.num = num;
            token.type = EXTTT_NUM;
            if (numSkipped) {
               if (!tokens.empty()) tokens.insert(tokens.end() - 1, token);
            } else tokens.push_back(token);
            prevWasOp = false;
         }
      }
      return tokens;
   }

   EXTreeNode *buildTree(const std::vector<EXTreeToken> &tokens) {
      size_t sFID = 0;
      int lim = 999999;
      int mLS = lim;
      int mOS = lim;
      int mPS = lim;
      if (tokens.empty()) return nullptr;
      for (size_t tIID = 0; tIID < tokens.size(); tIID++) {
         EXTreeToken token = tokens[tIID];
         if (token.type == EXTTT_OP) {
            int tLS = token.exOperator.layerScore;
            int tOS = 12 - getPrecedenceScore(token.exOperator.op);
            int tPS = token.exOperator.placeScore;

#define ANSMALL \
               mLS = tLS; \
               mOS = tOS; \
               mPS = tPS; \
               sFID = tIID;

            if (tLS <= mLS) {
               if (tLS < mLS) {
                  ANSMALL
               }
               if (tOS <= mOS) {
                  if (tOS < mOS) {
                     ANSMALL
                  }
                  if (tPS <= mPS) {
                     ANSMALL
                  }
               }
            }
         }
      }
      auto *node = new EXTreeNode();
      if (mLS == lim) {

         EXTreeToken token = tokens[0];
         if (token.type == EXTTT_NUM) {
            node->type = EXTTT_NUM;
            node->num = token.num;
         } else if (token.type == EXTTT_NAMEID) {
            node->type = EXTTT_NAMEID;
            node->valueID = token.valueID;
         } else return nullptr;
         return node;
      } else {
         std::vector<EXTreeToken> before;
         std::vector<EXTreeToken> after;
         before.assign(tokens.begin(), tokens.begin() + sFID);
         after.assign(tokens.begin() + sFID + 1, tokens.end());

         node->op = tokens[sFID].exOperator.op;
         node->type = EXTTT_OP;
         node->left = buildTree(before);
         node->right = buildTree(after);
         return node;
      }
   }

   void deleteTree(EXTreeNode *root) {
      if (!root) return;

      std::queue<EXTreeNode *> queue;

      if (root->type == EXTTT_OP) {
         queue.push(root->left);
         queue.push(root->right);
         root->right = nullptr;
         root->left = nullptr;
      }

      EXTreeNode *current;
      while (!queue.empty()) {
         current = queue.front();
         queue.pop();

         if (!current) continue;

         if (current->type == EXTTT_OP) {
            queue.push(current->left);
            queue.push(current->right);
            current->right = nullptr;
            current->left = nullptr;
         }

         delete current;
      }

      delete root;
      root = nullptr;
   }

   T evaluate(EXTreeNode *node, const std::vector<T> &values) {
      if (!node) return 1;
      if (node->type == EXTTT_NAMEID) {
         size_t nameID = node->valueID;
         if (nameID >= values.size()) return 1;
         return values[nameID];
      }
      if (node->type == EXTTT_NUM) return node->num;
      if (node->type == EXTTT_OP) {
         T leftVal = evaluate(node->left, values);
         T rightVal = evaluate(node->right, values);
         int leftIVal = (int) leftVal;
         int rightIVal = (int) rightVal;
         switch (node->op) {
            case OP_UNKNOWN:
               return -1;
            case OPA_NEG:
               return -rightIVal;
            case OPA_ADD:
               return leftVal + rightVal;
            case OPA_SUB:
               return leftVal - rightVal;
            case OPA_MUL:
               return leftVal * rightVal;
            case OPA_DIV:
               return leftVal / rightVal;
            case OPA_MOD:
               return fmodf(leftVal, rightVal);

            case OPC_EQUAL:
               return leftVal == rightVal;
            case OPC_NOTEQ:
               return leftVal != rightVal;
            case OPC_LESST:
               return leftVal < rightVal;
            case OPC_GREET:
               return leftVal > rightVal;
            case OPC_ELEST:
               return leftVal <= rightVal;
            case OPC_EGRET:
               return leftVal >= rightVal;

            case OPL_AND:
               return leftIVal && rightIVal;
            case OPL_NOT:
               return !rightIVal;
            case OPL_OR:
               return leftIVal || rightIVal;

            case OPB_BWAND:
               return leftIVal & rightIVal;
            case OPB_BWOR:
               return leftIVal | rightIVal;
            case OPB_BWXOR:
               return leftIVal ^ rightIVal;
            case OPB_BWNOT:
               return ~rightIVal;
            case OPB_BWLSH:
               return leftIVal << rightIVal;
            case OPB_BWRSH:
               return leftIVal >> rightIVal;

            default:
               return -1;
         }
      }
      return -1;
   }

   static int getPrecedenceScore(EXOP_En op) {
      static std::unordered_map<EXOP_En, int> precedenceMap = {
            // Arithmetic Operators
            {OPA_NEG,   1},
            {OPA_ADD,   3},
            {OPA_SUB,   3},
            {OPA_MUL,   2},
            {OPA_DIV,   2},
            {OPA_MOD,   2},
            // Comparison Operators
            {OPC_EQUAL, 6},
            {OPC_NOTEQ, 6},
            {OPC_LESST, 5},
            {OPC_GREET, 5},
            {OPC_ELEST, 5},
            {OPC_EGRET, 5},
            // Logical Operators
            {OPL_AND,   10},
            {OPL_OR,    11},
            {OPL_NOT,   1},
            // Bitwise Operators
            {OPB_BWAND, 7},
            {OPB_BWOR,  8},
            {OPB_BWXOR, 9},
            {OPB_BWNOT, 1},
            {OPB_BWLSH, 4},
            {OPB_BWRSH, 4}
      };

      auto it = precedenceMap.find(op);
      if (it != precedenceMap.end()) {
         return it->second;
      }
      return 0;
   }

};

#endif //GIT_CL_EXPRESSION_PARSER_EXPRESSION_PARSER_H
