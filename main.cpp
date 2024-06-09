#include <iostream>
#include "ExpressionParser.h"

int main() {
   //* This is an example project to demonstrate what the Header is capable,
   //* the program just expects a value and on entering an expression(calculation)
   //* it returns the calculated value also it can take in Variables.
   //* The parser also constructs an order of operations tree that can be useful,
   //* if u want to make a programing language or something.

   float PI = 3.14;

   ExpressionParser<float> parser;

   std::cout << "write !stop to exit" << std::endl;

   std::string inputStr;
   while(inputStr != "!stop") {
      std::cout << "input a expression also u can use PI" << std::endl;
      std::cin >> inputStr;

      parser.buildTree(inputStr, {"PI"});
      float value = parser.evaluate({PI});
      parser.deleteTree();

      std::cout << "Value: " << value << std::endl;
   }
   return 0;
}
