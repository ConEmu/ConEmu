// SPDX-License-Identifier: BSD-3-Clause
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cstdio>
#include "../../common/UnicodeGrapheme.h"
int wmain(int argc,wchar_t**argv){if(argc!=2)return 2;std::ifstream file(argv[1]);if(!file)return 3;std::string line;unsigned cases=0,boundaries=0,failed=0;
 while(std::getline(file,line)){line=line.substr(0,line.find('#'));std::istringstream input(line);std::string token;std::vector<unsigned> cluster;bool expected=true,any=false;
  while(input>>token){if(token=="\xC3\xB7"){expected=true;continue;}if(token=="\xC3\x97"){expected=false;continue;}unsigned cp=std::stoul(token,nullptr,16);bool actual=conpty_unicode::BreakBefore(cluster,cp);boundaries++;any=true;if(actual!=expected){failed++;printf("FAIL case %u U+%04X\n",cases+1,cp);}if(expected)cluster.clear();cluster.push_back(cp);}
  cases+=any;
 }
 printf("%s Unicode 17 GraphemeBreakTest: %u cases, %u boundaries, %u failures\n",failed?"FAIL":"PASS",cases,boundaries,failed);return failed?1:0;}
