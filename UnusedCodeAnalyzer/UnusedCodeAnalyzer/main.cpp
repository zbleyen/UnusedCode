//
//  main.m
//  UnusedCodeAnalyzer
//
//  Created by 张博 on 2018/2/25.
//  Copyright © 2018年 张博. All rights reserved.
//

#include <iostream>
#include <string>
#include <cstdio>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <sstream>
#include <vector>
#include <fstream>
#include <iterator>
#include "json.hpp"
#include "dijkstra.cpp"

using namespace std;
using namespace nlohmann;

const static string usedCountKey =   "_usedcount";
static json usedJson;

string exec(const char* cmd) {
    char buffer[128];
    string result = "";
    shared_ptr<FILE> pipe(popen(cmd, "r"), pclose);
    if (!pipe) throw std::runtime_error("popen() failed!");
    while (!feof(pipe.get())) {
        if (fgets(buffer, 128, pipe.get()) != NULL)
            result += buffer;
    }
    return result;
}

vector<string> split(const string &s, char delim) {
    vector<string> elems;
    stringstream ss;
    ss.str(s);
    string item;
    while (getline(ss, item, delim)) {
        elems.push_back(item);
    }
    return elems;
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

json readJsonFromFile(const string& file){
    ifstream ifs(file);
    string str((istreambuf_iterator<char>(ifs)),
               istreambuf_iterator<char>());
    return json::parse(str);
}

bool writeJsonToFile(json j, string filename){
    if(j.is_null()){
        return false;
    }
    ofstream ofs;
    ofs.open (filename,ofstream::out);
    ofs<<j<<endl;
    //    cout<<filename<<":"<<endl;
    //    cout<<j<<endl;
    ofs.close();
    return true;
}

bool writeToFileAtBegin(string content, string filename) {
    fstream fs;
    string oldContent;
    fs.open (filename,std::fstream::in | std::fstream::out);
    fs >> oldContent;
    fs << content + oldContent;
    fs.close();
    return true;
}

bool writeToFileAtEnd(string content, string filename) {
    ofstream ofs;
    ofs.open (filename,ofstream::app);
    ofs<<content<<endl;
    ofs.close();
    return true;
}

template<class T>
vector<T> mergeVector(vector<T> vec1,vector<T> vec2){
    vector<T> mergedVec;
    mergedVec.reserve(vec1.size() + vec2.size() );
    mergedVec.insert( mergedVec.end(), vec1.begin(), vec1.end() );
    mergedVec.insert( mergedVec.end(), vec2.begin(), vec2.end() );
    sort( mergedVec.begin(), mergedVec.end() );
    mergedVec.erase(unique( mergedVec.begin(), mergedVec.end() ), mergedVec.end() );
    return mergedVec;
}



vector<string> jsonPartFiles(string folderDir,string suffix){
    stringstream ss;
    ss<<"find "<<folderDir<<" -type f -name \"*."<<suffix<<"\"";
    string findOpt = exec(ss.str().c_str());
    return split(findOpt,'\n');
}

string firstCommitterOfFile(string folderDir,string path) {
    stringstream ss;
    ss<<"git log --pretty=format:\"%an\" -- "<<path;
    string all = exec(ss.str().c_str());
    return split(all, '\n').back();
}

// whiteList :  vector<string> files = jsonPartFiles(folder,"classWhiteList.json");
// all :        vector<string> files = jsonPartFiles(folder,"classList.json");
vector<string> mergeVectorJsonFiles (vector<string> files){
    vector<string> mergedVec;
    for(vector<string>::iterator it=files.begin();it!=files.end();++it){
        json jpj = readJsonFromFile(*it);
        vector<string> cls = jpj.get<vector<string>>();
         mergedVec.insert( mergedVec.end(), cls.begin(), cls.end() );
    }
    sort( mergedVec.begin(), mergedVec.end() );
    mergedVec.erase(unique( mergedVec.begin(), mergedVec.end() ), mergedVec.end() );
    return mergedVec;
}
// callee   :   jsonPartFiles(folder,"classCallees.json");
json mergeMapJsonFiles (vector<string> files){
    json allJson;
    for(vector<string>::iterator it=files.begin();it!=files.end();++it){
        json jpj = readJsonFromFile(*it);
        if(jpj.is_null())
            continue;
        for (json::iterator it = jpj.begin(); it != jpj.end(); ++it) {
            string key = it.key();
            json value = it.value();
            if(allJson.find(key)==allJson.end()){
                allJson[key]=value;
            } else {
                json mergedValue = json(allJson[key]);
                for (json::iterator it = value.begin(); it != value.end(); ++it) {
                    string key = it.key();
                    int v = it.value();
                    if(mergedValue.find(key)==mergedValue.end()){
                        mergedValue[key]=v;
                    } else {
                        mergedValue[key] = (int)(mergedValue[key]) + v;
                    }
                }
                allJson[key] = mergedValue;
            }
        }
    }
    return allJson;
}

json mergeFilePathJson(vector<string> files){
    json allJson;
    for(vector<string>::iterator it=files.begin();it!=files.end();++it){
        json jpj = readJsonFromFile(*it);
        if(jpj.is_null())
            continue;
        for (json::iterator it = jpj.begin(); it != jpj.end(); ++it) {
            string key = it.key();
            json value = it.value();
            allJson[key]=value;
        }
    }
    return allJson;
}

bool callIsCategoryMethod(json category, const string &className, const string &callee, string &cateName ) {
    string keyPrefix = className + "(";
    for (json::iterator it = category.begin(); it != category.end(); ++it) {
        string key = it.key();
        if (has_prefix(key, keyPrefix)) {
            json cate = it.value();
            for (json::iterator it = cate.begin(); it != cate.end(); ++it) {
                string call = it.key();
                if (!callee.compare(call)) {
                    cateName = key;
                    return true;
                }
            }
        }
        
    }
    return false;
}


json preprocessCallees(json callees, json category) {
    json retCallees;
    for (json::iterator it = callees.begin(); it != callees.end(); ++it) {
        string key = it.key();
        json callee = it.value();
        json newCallee;
        for (json::iterator it = callee.begin(); it != callee.end(); ++it) {
            string call = it.key();
            int referenceCount = it.value();
            string newKey = call;
            if (has_prefix(call, "-[") || has_prefix(call, "+[")) {
                string className = "";
                size_t pos = call.find(" ");
                if(pos!=string::npos){
                    className = call.substr(2,pos -2);
                }
                string cateName;
                if (callIsCategoryMethod(category, className, call,cateName)) {
                    newKey = cateName;
                } else {
                    newKey = className;
                }
            }
            json accumulatedRefrenceCount = newCallee[newKey];
            if (accumulatedRefrenceCount.is_null()) {
                newCallee[newKey] = referenceCount;
            } else {
                newCallee[newKey] = referenceCount + (int)accumulatedRefrenceCount;
            }
        }
        retCallees[key] = newCallee;
    }
    return retCallees;
}

string getFileAuthor(string folder,string path) {
    ifstream ifs(path);
    string str((istreambuf_iterator<char>(ifs)),
               istreambuf_iterator<char>());
    string prefix = str.substr(0,100);
    string leftStr = "Created by ";
    string rightStr = " on ";
    size_t pos1 = prefix.find(leftStr);
    size_t pos2 = prefix.find(rightStr);
    if(pos1!=string::npos && pos2 != string::npos){
        size_t start = pos1 + leftStr.length();
        return prefix.substr(start,pos2 - start);
    }
    return firstCommitterOfFile(folder,path);
}

json getFileAuthorJson(string folder,json filePath) {
    json author;
    for (json::iterator it = filePath.begin(); it != filePath.end(); ++it) {
        string key = it.key();
        string path = it.value();
        author[key] = getFileAuthor(folder, path);
    }
    return author;
}

json reduceJson(json mapJson, vector<string> vec) {
//    cout <<vec<<endl;
    json reducedJson;
    for(vector<string>::iterator it=vec.begin();it!=vec.end();++it){
        reducedJson[*it] = mapJson[*it];
//        cout<<reducedJson<<endl;
//        cout<<*it<<endl;
    }
    return reducedJson;
}

void increaseUsedCount(json callees,string className,int usedCount) {
    json classJson = callees[className];
    if (classJson.is_null()) {
        usedJson[className] = usedCount;
        return;
    }
//    cout << className <<" used"<<endl;
    int usedCountOld = usedJson[className];
    if (usedCountOld == 0) {
        usedJson[className] = usedCount;
        for (json::iterator it = classJson.begin(); it != classJson.end(); ++it) {
            string key = it.key();
            int value = it.value();
            increaseUsedCount(callees, key,value);
        }
    } else {
        usedJson[className] = usedCountOld + usedCount;
    }
}


vector<string> unusedClass (json callees,vector<string> whiteList){
    
    
    vector<string> unusedClassVec;
    for(vector<string>::iterator it=whiteList.begin();it!=whiteList.end();++it){
        increaseUsedCount(callees, *it, 1);
    }
    for (json::iterator it = usedJson.begin(); it != usedJson.end(); ++it) {
        string key = it.key();
        int value = it.value();
        if (value == 0) {
            unusedClassVec.push_back(key);
        }
    }
    return unusedClassVec;
}



int main(int argc, const char * argv[]) {
    if(argc!=2){
        cout<<"Usage:"<<"postanalyze your-path-of-jsonparts-for-analyzing your-appdelegate-name"<<endl;
        return -1;
    }
    string folder(argv[1]);

    //callee
    json callees = mergeMapJsonFiles(jsonPartFiles(folder,"classCallees.json"));

    //preprocess category
    json category = mergeMapJsonFiles(jsonPartFiles(folder, "category.json"));

    cout << "category:" << category <<endl;
    cout <<"_______"<<endl;
    callees = preprocessCallees(callees,category);
    
    //whiteList
    vector<string> whiteListClass = mergeVectorJsonFiles(jsonPartFiles(folder,"classWhiteList.json"));
    
    // produce edges of calling relation graph for d3js visualization
    json edges = edgesForD3js(callees, whiteListClass);
    writeJsonToFile(edges, folder + "/origin.js");
    writeToFileAtBegin("var dependencies = ", folder + "/origin.js");
    writeToFileAtEnd(";", folder + "/origin.js");

    //put main() in whiteList
    whiteListClass.push_back(string("main()"));
    
    // init usedCount
    for (json::iterator it = callees.begin(); it != callees.end(); ++it) {
        string key = it.key();
        usedJson[key] = 0;
    }

    //analyze
    cout << "whiteListClass:" <<json(whiteListClass)<<endl;
    cout <<"_______"<<endl;
    cout << "callees:" << callees <<endl;
    vector<string> unusedClassVec = unusedClass(callees, whiteListClass);
    writeJsonToFile(json(unusedClassVec), folder + "/unusedClass.json");
    //上面基本分析已经完成
    
    //分析各类的创建者，方便人工筛查
    json filePathJson = mergeFilePathJson(jsonPartFiles(folder,"filePath.json"));
    cout<< "filePathJson: "<<filePathJson<<endl;
    json fileAuthorJson = getFileAuthorJson( folder,reduceJson(filePathJson, unusedClassVec));
    cout<< "fileAuthorJson: "<<fileAuthorJson<<endl;
    writeJsonToFile(fileAuthorJson, folder + "/unusedClassWithAuthor.json");
    return 0;
}
