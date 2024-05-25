#include <vector>
#include <iostream>
#include <string>

#include "KeyWordSearch.h"
#include "ExpressionParser.h"

int main() {

   ExpressionParser<float> parser;
   parser.buildTree("x*y*2-8",{"x","y"});
   std::cout << parser.evaluate({2,4});

   /*
   There are some problems the -5 or -var
   dose not work. So if u want to work with negative numbers
   just do (0-x) or (0-78) that should work fine.

   The other issue is if the to variables contain each other 
   I mean do not create a value and a val variable, becouse value contains val.
   */
};