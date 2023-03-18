//
//  UnusedCodeUtil.cpp
//  UnusedCode
//
//  Created by 张博 on 2018/2/9.
//  Copyright © 2018年 张博. All rights reserved.
//

#include "UnusedCodeUtil.hpp"

string gSrcRootPath = "";
string gFileName = "";
string gRightHeaderFileName = "";

string classWhiteList = "classWhiteList";
string classCallees = "classCallees";


json classWhiteListJson;
//json classListJson;
//json classCalleesJson;
json functionCalleesJson;
json relatedClassesJson;    //[类名：[相关类名:次数]]
json currentClassJson;
json wholeJson;
json categoryImpJson;

vector<string>  classCalleesVector;

string className;       //如果是objc类中的方法（ObjCMethodDecl），以类名建索引
string functionName;    //如果是FunctionDecl，以方法名建索引



string removeBackSlash(string s) {
    string str(s);
    string cs2Remove = "/\\";
    for (unsigned int i = 0; i < cs2Remove.length(); ++i)
        str.erase (remove(str.begin(), str.end(), cs2Remove.at(i)), str.end());
    return str;
}
void clsVisitEnd(string clsName) {
    currentClassJson[string("methods")] = functionCalleesJson;
    currentClassJson[string("relatedClasses")] = relatedClassesJson[clsName];
    wholeJson[className] = currentClassJson;
    functionCalleesJson.clear();
    currentClassJson.clear();
}

void UnusedCodeUtil::setClassName(string clsName){
    if(!clsName.length())
        return;
    if (className.length() && className.compare(clsName)) {
        clsVisitEnd(className);
    }
    className = clsName;
}

void UnusedCodeUtil::setRelatedClsWithClass(string relatedCls, string thisCls) {
    json relatedJson = relatedClassesJson[thisCls];
    if (relatedJson.is_null()) {
        relatedJson[relatedCls] = 1;
    }
    relatedClassesJson[thisCls] = relatedJson;
}


void UnusedCodeUtil::setSuperClassName(string spClsName) {
    currentClassJson[string("superClass")] = spClsName;
}


void UnusedCodeUtil::setFunctionName(string funcName) {
    functionName = funcName;
//    cout<<"setFunctionName:"<<funcName<<endl;
}

bool UnusedCodeUtil::writeJsonToFile(json j, string filename){
    if(j.is_null()){
        return false;
    }
    ofstream ofs;
    ofs.open (filename,ofstream::out);
    ofs<<j<<endl;
//    cout<<"zbdebug fileName :"<<filename<<endl;
//    cout<<j<<endl;
    ofs.close();
    return true;
}

void increaseUsedCount(json &j, string key) {
    if (j.find(key) == j.end()) {
        j[key] = 1;
    } else {
        j[key] = (int)(j[key]) + 1;
    }
}

void UnusedCodeUtil::appendCalleeClass(string callee) {
    
    if (functionName.length()) {
        json funcCallees = functionCalleesJson[functionName];
        if (funcCallees.is_null()) {
            funcCallees[callee] = 1;
            
        } else {
            increaseUsedCount(funcCallees, callee);
        }
        functionCalleesJson[functionName] = funcCallees;
        return;
    }
//    increaseUsedCount(classCalleesJson,callee);

}

void UnusedCodeUtil:: appendCategoryMethod(string mtdSelector) {
    if (!className.length() || !mtdSelector.length()) {
        return;
    }
    json cateJson = categoryImpJson[className];
    cateJson[mtdSelector] = 1;//value无实际意义，只是需要字典结构
    categoryImpJson[className] = cateJson;
}

void UnusedCodeUtil::appendWhiteListClass(string cls) {
//    if (classWhiteListJson.find(cls) == classWhiteListJson.end()) {
        classWhiteListJson.push_back(cls);
//    }
}

//void UnusedCodeUtil::appendClass(string cls) {
//    classListJson.push_back(cls);
//}

void UnusedCodeUtil::synchronize(){
    string fileprefix = string(removeBackSlash(gFileName));
    if(!fileprefix.length()){
        ofstream ofs;
        ofs.open (gSrcRootPath+"/Analyzer/"+"error.json",ofstream::out);
        ofs<<"Error:Filename not set."<<endl;
        ofs.close();
        cout<<"[KWLM]:Nofilename"<<endl;
    }
    if (className.length()) {
        clsVisitEnd(className);
    }
    
    //记录文件路径
    json filePathJson;
    string fileName = lastComponentOfFile(gFileName);
    filePathJson[fileName] = gFileName;
    
    writeJsonToFile(filePathJson, gSrcRootPath+"/Analyzer/"+fileprefix+".filePath.json");
    json fileJson;
    fileJson[fileName] = wholeJson;
    writeJsonToFile(fileJson, gSrcRootPath+"/Analyzer/"+fileprefix+".classCallees.json");

//    cout <<"fileName: "<< gSrcRootPath+"/Analyzer/"+fileprefix+".classCallees.json"<<endl;
//    writeJsonToFile(classWhiteListJson, gSrcRootPath+"/Analyzer/"+fileprefix+".classWhiteList.json");
//    writeJsonToFile(categoryImpJson, gSrcRootPath + "/Analyzer/" + fileprefix + ".category.json");
//    writeJsonToFile(classListJson, gSrcRootPath+"/Analyzer/"+fileprefix+".classList.json");
}
