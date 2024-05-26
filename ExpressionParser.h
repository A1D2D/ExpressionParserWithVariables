#pragma once

#include <cmath>
#include "KeyWordSearch.h"

template <typename T>
class ExpressionParser {
 private:
   enum EXOP_En {
      OP_UNKOWN = 'u',
      //Arithmetic Operators
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
      OPL_NOT = 'n',
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

      ExOperator() : op(OP_UNKOWN), placeScore(0), layerScore(0) {}

      ExOperator(EXOP_En op, int placeScore, int layerScore)
         : op(op), placeScore(placeScore), layerScore(layerScore) {}
   };

   struct EXTreeNode {  
      EXTreeTokenType type;
      union {
         struct {
            EXOP_En op;
            EXTreeNode* left = nullptr;
            EXTreeNode* right = nullptr;
         };
         size_t valueID;
         T num;
      };

      EXTreeNode() : type(EXTTT_OP) {
      }
      /*~EXTreeNode() {
         if(type == EXTTT_OP) {
            delete left;
            delete right;
         }
      }*/
   };

   struct EXTreeToken {
      EXTreeTokenType type;
      union {
         ExOperator exOperator;
         size_t valueID;
         T num;
      };
      EXTreeToken() : type(EXTTT_OP) {
        new (&exOperator) ExOperator();
      }
      ~EXTreeToken() {
         exOperator.~ExOperator();
      }

   };

 public:
   ExpressionParser() {
   }

   ExpressionParser(const std::string& expression) {
      exprSTR = expression;
   }

   ~ExpressionParser() {
      exprTokens.clear();
      exprSTR = "";
      deleteTree();
   }

   void buildTree(std::string expression,const std::vector<std::string>& inputs) {
      exprTokens = tokenizeExpression(expression,inputs);
      rootNode = buildTree(exprTokens);
   }

   void buildTree(const std::vector<std::string>& inputs) {
      exprTokens = tokenizeExpression(exprSTR,inputs);
      rootNode = buildTree(exprTokens);
   }

   T evaluate(const std::vector<T>& values) {
      return evaluate(rootNode,values);
   }

   void deleteTree() {
      deleteTree(rootNode);
   }

   std::string exprSTR = "";
   EXTreeNode* rootNode = nullptr;
   std::vector<EXTreeToken> exprTokens;

 private:
   bool isDigit(char c) {
      return std::isdigit(static_cast<unsigned char>(c));
   }

   EXOP_En getEXOPEnum(const std::string& ch,int& id) {
      char mainCh = ch[id];
      bool secUV = (id <= ch.size()-1);
      switch (mainCh) {
         case '+':
            return OPA_ADD;
         case '-':
            return OPA_SUB;
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
            if(secUV) return OPL_NOT;
            if (ch[id+1] == '=') {
               id++;
               return OPC_NOTEQ;
            } else return OPL_NOT;
         case '<':
            if(secUV) return OPC_LESST;
            if (ch[id+1] == '=') {
               id++;
               return OPC_ELEST;
            } else if (ch[id+1] == '<') {
               id++;
               return OPB_BWLSH;
            } else return OPC_LESST;
         case '>':
            if(secUV) return OPC_GREET;
            if (ch[id+1] == '=') {
               id++;
               return OPC_EGRET;
            } else if (ch[id+1] == '>') {
               id++;
               return OPB_BWRSH;
            } else return OPC_GREET;
         case '&':
            if(secUV) return OPB_BWAND;
            if (ch[id+1] == '&') {
               id++;
               return OPL_AND;
            } else return OPB_BWAND;
         case '|':
            if(secUV) return OPB_BWOR;
            if (ch[id+1] == '|') {
               id++;
               return OPL_OR;
            } else return OPB_BWOR;

         default:
            return OP_UNKOWN;
      }
   }

   std::vector<EXTreeToken> tokenizeExpression(const std::string& expression,const std::vector<std::string>& inputs) {
      std::vector<EXTreeToken> tokens;
      KeyWordSearch search;
      for (int iID = 0; iID < inputs.size(); iID++) {
         search.addKeyWord(inputs[iID]);
      }
      search.machCase = true;
      search.firstElement = false;
      search.buildTrie();
      std::vector<KeyWordSearch::FoundKeyWord> foundKeys = search.searchForKeywords(expression);
      search.deleteTrie();

      int OPID = 0;
      int opDepth = 0;
      int nextKey = 0;

      //numEx
      T snum = 0;
      int snumL = 0;
      int dot = 0;
      bool isNum = false;
      bool hasDot = false;

      for (int sID = 0; sID < expression.size(); sID++) {
         bool jumped = false;
         bool numSkipped = false;
         if(expression[sID] == ' ') {
            jumped = true;
            goto skipNumAdd;
         } 
         if(expression[sID] == '(') {
            opDepth++;
            jumped = true;
            goto skipNumAdd;
         } 
         if(expression[sID] == ')') {
            opDepth--;
            jumped = true;
            goto skipNumAdd;
         }
         {
            bool interPrew = foundKeys.size() > 0&&sID == foundKeys[nextKey].position;
            if(interPrew) {
               sID += foundKeys[nextKey].length-1;
               EXTreeToken token;
               token.valueID = foundKeys[nextKey++].id;
               token.type = EXTTT_NAMEID;
               tokens.push_back(token);
               jumped = true;
               numSkipped = true;
               goto skipNumAdd;
            }
            EXOP_En expr = getEXOPEnum(expression,sID);
            if(expr != OP_UNKOWN) {
               ExOperator exOperator;
               exOperator.layerScore = opDepth;
               exOperator.op = expr;
               exOperator.placeScore = expression.length()-OPID++;

               EXTreeToken token;
               token.exOperator = exOperator;
               token.type = EXTTT_OP;
               tokens.push_back(token);
               jumped = true;
               numSkipped = true;
               goto skipNumAdd;
            }
            if(isDigit(expression[sID]) || expression[sID] == '.') {
               if (expression[sID] == '.') {
                  dot = snumL;
                  hasDot = true;
               } else {
                  isNum = true;
                  snum*=10;
                  snum += (expression[sID]-48);
                  snumL++;
               }
            }
         }

         skipNumAdd:
         bool numEnd = !(isDigit(expression[sID+1]) || expression[sID+1] == '.');
         bool exprEnd = sID >= expression.size()-1;
         if(isNum && (numEnd || exprEnd || jumped)) {
            T num = snum;
            T div = powf(10,snumL-dot);
            if(hasDot)num /= div;
            snum = 0;
            snumL = 0;
            dot = 0;
            isNum = false;
            hasDot = false;

            
            EXTreeToken token;
            token.num = num;
            token.type = EXTTT_NUM;
            if(numSkipped) {
               if (!tokens.empty()) tokens.insert(tokens.end() - 1, token);
            } else tokens.push_back(token);
         }
      }
      return tokens;
   }

   EXTreeNode* buildTree(const std::vector<EXTreeToken>& tokens) {
      size_t sFID = 0;
      int lim = 999999;
      int mLS = lim;
      int mOS = lim;
      int mPS = lim;
      if(tokens.empty()) return nullptr;
      for (size_t tIID = 0; tIID < tokens.size(); tIID++) {
         EXTreeToken token = tokens[tIID];
         if(token.type == EXTTT_OP) {
            int tLS = token.exOperator.layerScore;
            int tOS = 12-getPrecedenceScore(token.exOperator.op);
            int tPS = token.exOperator.placeScore;

            #define ANSMALL \
               mLS = tLS; \
               mOS = tOS; \
               mPS = tPS; \
               sFID = tIID; 

            if(tLS <= mLS) {
               if(tLS < mLS) {
                  ANSMALL
               }
               if(tOS <= mOS) {
                  if(tOS < mOS) {
                     ANSMALL
                  }
                  if(tPS <= mPS) {
                     ANSMALL
                  }
               }
            }
         }
      }
      EXTreeNode* node = new EXTreeNode();
      if(mLS == lim) {
         
         EXTreeToken token = tokens[0];
         if (token.type == EXTTT_NUM) {
            node->type = EXTTT_NUM;
            node->num = token.num;
         } else if(token.type == EXTTT_NAMEID) {
            node->type = EXTTT_NAMEID;
            node->valueID = token.valueID;
         } else return nullptr;
         return node;
      } else {
         std::vector<EXTreeToken> before;
         std::vector<EXTreeToken> after;
         before.assign(tokens.begin(), tokens.begin() + sFID);
         after.assign(tokens.begin() + sFID+1, tokens.end());

         node->op = tokens[sFID].exOperator.op;
         node->type = EXTTT_OP;
         node->left = buildTree(before);
         node->right = buildTree(after);
         return node; 
      }
   }

   void deleteTree(EXTreeNode* root) {
      if(!root) return;

      std::queue<EXTreeNode*> queue;

      if(root->type == EXTTT_OP) {
         queue.push(root->left);
         queue.push(root->right);
         root->right = nullptr;
         root->left = nullptr;
      }

      EXTreeNode* current;
      while (!queue.empty()) {
         current = queue.front();
         queue.pop();

         if(!current) continue;

         if(current->type == EXTTT_OP) {
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

   T evaluate(EXTreeNode* node,const std::vector<T>& values) {
      if(!node) return 1;
      if(node->type == EXTTT_NAMEID) {
         size_t nameID = node->valueID;
         if(nameID >= values.size()) return 1;
         return values[nameID];
      }
      if(node->type == EXTTT_NUM) return node->num;
      if(node->type == EXTTT_OP) {
         T leftVal = evaluate(node->left,values);
         T rightVal = evaluate(node->right,values);
         int leftIVal = (int) leftVal;
         int rightIVal = (int) rightVal;
         switch (node->op) {
            case OP_UNKOWN:
               return -1;
            case OPA_ADD:
               return leftVal + rightVal;
            case OPA_SUB:
               return leftVal - rightVal;
            case OPA_MUL:
               return leftVal * rightVal;
            case OPA_DIV:
               return leftVal / rightVal;
            case OPA_MOD:
               return fmodf(leftVal,rightVal);

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
         {OPA_ADD, 3},
         {OPA_SUB, 3},
         {OPA_MUL, 2},
         {OPA_DIV, 2},
         {OPA_MOD, 2},
         // Comparison Operators
         {OPC_EQUAL, 6},
         {OPC_NOTEQ, 6},
         {OPC_LESST, 5},
         {OPC_GREET, 5},
         {OPC_ELEST, 5},
         {OPC_EGRET, 5},
         // Logical Operators
         {OPL_AND, 10},
         {OPL_OR, 11},
         {OPL_NOT, 1},
         // Bitwise Operators
         {OPB_BWAND, 7},
         {OPB_BWOR, 8},
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
