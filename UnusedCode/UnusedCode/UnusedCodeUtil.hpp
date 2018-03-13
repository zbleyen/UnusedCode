//
//  UnusedCodeUtil.hpp
//  UnusedCode
//
//  Created by 张博 on 2018/2/9.
//  Copyright © 2018年 张博. All rights reserved.
//

#ifndef UnusedCodeUtil_hpp
#define UnusedCodeUtil_hpp

#include <string>
#include <fstream>
#include <vector>
#include <map>
#include "json.hpp"

#define kAppMainEntryClass  "UIApplication"

using namespace std;
using namespace nlohmann;

extern string gSrcRootPath;
extern string gFileName;

static vector<string> split(const string &s, char delim) {
    vector<string> elems;
    stringstream ss;
    ss.str(s);
    string item;
    while (getline(ss, item, delim)) {
        elems.push_back(item);
    }
    return elems;
}

static inline void remove_blank(string &str)
{
    str.erase(remove(str.begin(), str.end(), ' '), str.end());
    str.erase(remove(str.begin(), str.end(), '\t'), str.end());
}

static inline bool has_suffix(const std::string &str, const std::string &suffix)
{
    return str.size() >= suffix.size() &&
    str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

static inline bool has_prefix(const std::string &str, const std::string &prefix)
{
    return str.size() >= prefix.size() &&
    str.compare(0, prefix.size(), prefix) == 0;
}

static inline string &ltrim(std::string &s) {
    s.erase(s.begin(), find_if(s.begin(), s.end(),
                               not1(ptr_fun<int, int>(isspace))));
    return s;
}

// trim from end
static inline string &rtrim(string &s) {
    s.erase(find_if(s.rbegin(), s.rend(),
                    not1(ptr_fun<int, int>(isspace))).base(), s.end());
    return s;
}

// trim from both ends
static inline string &trim(string &s) {
    return ltrim(rtrim(s));
}

static inline string absolutePathFromRelative(string relativePath){
    vector<string> pathComs = split(relativePath,'/');
    vector<string> absoPathVec;
    ostringstream oss;
    for(vector<string>::iterator it = pathComs.begin();it!=pathComs.end();it++){
        string path = *it;
        if(!path.compare("") || !path.compare("."))
            continue;
        if(!path.compare("..")){
            absoPathVec.pop_back();
        }
        else{
            absoPathVec.push_back(string("/")+path);
        }
    }
    copy(absoPathVec.begin(), absoPathVec.end(), ostream_iterator<string>(oss));
    return oss.str();
}

class UnusedCodeUtil{
public:
//    static void setFilename(string fname,bool forceSet);
//    static string getFileName();
    static void setClassName(string clsName);
    static void setFunctionName(string funcName);
    
    static void appendCalleeClass(string cls);
    static void appendCategoryMethod(string mtdSelector);
    static void appendWhiteListClass(string cls);
//    static void appendClass(string cls);
    static bool writeJsonToFile(json j,string filename);
    static void synchronize();
};


#endif /* UnusedCodeUtil_hpp */
