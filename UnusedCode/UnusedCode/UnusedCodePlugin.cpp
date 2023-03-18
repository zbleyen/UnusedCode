//
//  UnusedCodePlugin.cpp
//  UnusedCode
//
//  Created by 张博 on 2018/2/9.
//  Copyright © 2018年 张博. All rights reserved.
//

#include "UnusedCodePlugin.hpp"
#include <iostream>
#include "clang/AST/AST.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Lex/Lexer.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
// Declares llvm::cl::extrahelp.
#include "llvm/Support/CommandLine.h"


using namespace clang;
using namespace std;
using namespace llvm;
using namespace clang::tooling;


string contentOfObjcString(string objcString) {
    if (objcString.length() < 4) {
        return "";
    }
    return objcString.substr(2,objcString.length() - 3);
}

string stringOfObjcMessage(ObjCMessageExpr *objcMessage) {
    string selectorName =   objcMessage->getSelector().getAsString();
//    cout <<"selectorName : "<<selectorName;
    ObjCInterfaceDecl *interface = objcMessage->getReceiverInterface();
    string interfaceName;
    if (interface) {
         interfaceName = interface->getNameAsString();
//        cout << "  interfaceName: "<<interfaceName<<endl;
    } else {
        string receiverType =objcMessage->getReceiverType().getAsString();
        size_t pos = receiverType.find(" ");
        if(pos!=string::npos){
            interfaceName = receiverType.substr(0,pos);
        }
//        cout <<" receiverType: "<<receiverType<<endl;
    }
    bool isInstanceMethod = objcMessage->isInstanceMessage();
    return string(isInstanceMethod?"-":"+") + "[" + interfaceName + " " + selectorName + "]";
    
    
}

string stringOfObjCMethod(ObjCMethodDecl *decl) {
    ObjCMethodDecl *methodDecl = (ObjCMethodDecl *)decl;
    bool isInstanceMethod = methodDecl->isInstanceMethod();
    string selectorName = methodDecl->getSelector().getAsString();
    string interfaceName = methodDecl->getClassInterface()->getNameAsString();
    return string(isInstanceMethod?"-":"+") + "[" + interfaceName + " " + selectorName + "]";
}

string stringOfObjCCategoryImplDecl(ObjCCategoryImplDecl *decl) {
    ObjCCategoryDecl *categoryDecl = decl->getCategoryDecl();
    ObjCInterfaceDecl *interfaceDecl = decl->getClassInterface();
    string cate = categoryDecl->getNameAsString();
    string cls = interfaceDecl->getNameAsString();
    return cls +"(" + cate + ")";
//    return cls;
}

namespace UnusedCodePlugin
{
    class UnusedCodeASTVisitor : public RecursiveASTVisitor<UnusedCodeASTVisitor>
    {
    private:
        ASTContext *context;
    public:
        void setContext(ASTContext &context){
            this->context = &context;
        }
        
        bool VisitDecl(Decl *decl){

            //@implementation
            static ObjCImplementationDecl *lastImpDecl ;
            static ObjCCategoryImplDecl *lastCategoryImpDecl;
            static ObjCInterfaceDecl *lastInterfaceDecl;
            static string lastClassName =string("");
            if (isa<ObjCImplementationDecl>(decl))
            {
                lastInterfaceDecl = NULL;
                lastImpDecl = (ObjCImplementationDecl *)decl;
                UnusedCodeUtil::setClassName(lastImpDecl->getNameAsString());
                lastClassName = lastImpDecl->getNameAsString();
                lastCategoryImpDecl = NULL;
                //继承
                if (lastImpDecl->getClassInterface()) {
                    ObjCInterfaceDecl *interfaceDecl = lastImpDecl->getClassInterface();
                    if (interfaceDecl->getSuperClass()) {
                        UnusedCodeUtil::setSuperClassName(interfaceDecl->getSuperClass()->getNameAsString());
                    }
                }
            }
            if (isa<ObjCCategoryImplDecl>(decl)) {
                lastInterfaceDecl = NULL;
                lastCategoryImpDecl = (ObjCCategoryImplDecl *)decl;
                UnusedCodeUtil::setClassName((stringOfObjCCategoryImplDecl(lastCategoryImpDecl)));
            }
            
            if (isa<ObjCInterfaceDecl>(decl)) {
                lastInterfaceDecl = (ObjCInterfaceDecl *)decl;
            }
            
//            if(isa<ObjCImplDecl>(decl)){
//                ObjCImplDecl *interDecl = (ObjCImplDecl *)decl;
//                fileName = context->getSourceManager().getFilename(decl->getSourceRange().getBegin()).str();
//                UnusedCodeUtil::setFilename(fileName,true);
//                cout <<"setFileName in ObjCImplDecl"<<fileName<<endl;
//            }
            
            //+ (void)load
            if(isa<ObjCMethodDecl>(decl)){
                ObjCMethodDecl *methodDecl = (ObjCMethodDecl *)decl;
                bool objcIsInstanceMethod = methodDecl->isInstanceMethod();
                string objcSelector = methodDecl->getSelector().getAsString();
                if (!objcIsInstanceMethod && !objcSelector.compare("load")) {
                    string className = methodDecl->getClassInterface()->getNameAsString();
//                    cout <<"VisitDecl |+ (void)load |className: "<<className<<endl;
                    if (lastImpDecl && !className.compare(lastClassName)) {
                        UnusedCodeUtil::appendWhiteListClass(lastClassName);
                        lastImpDecl = NULL;
                    }
                }
                if (methodDecl->hasBody()) {
                    UnusedCodeUtil::setFunctionName(stringOfObjCMethod(methodDecl));
                }
                if (lastCategoryImpDecl) {
                    UnusedCodeUtil::appendCategoryMethod(stringOfObjCMethod(methodDecl));
                }
//                cout<<"visitDecl ObjCMethodDecl "<<methodDecl->getNameAsString()<<endl;
            }
            //function
            if (isa<FunctionDecl>(decl)) {
                FunctionDecl *funcDecl = (FunctionDecl *)decl;
                Stmt *funcBody = funcDecl->getBody();
                if (funcBody) {
                    string funcSrcFile =  this->context->getSourceManager().getFilename(funcBody->getSourceRange().getBegin()).str();
                    if (!funcSrcFile.compare(gFileName)) {
                        UnusedCodeUtil::setFunctionName(funcDecl->getNameAsString() + "()");
                    }
                }
//                cout<<"visitDecl FunctionDecl "<<funcDecl->getNameAsString()<<endl;
            }
//            if (isa<VarDecl>(decl)) {
//                VarDecl *varDecl = (VarDecl *)decl;
//                if ((lastImpDecl||lastCategoryImpDecl) && varDecl->getType()->isAnyPointerType()) {
//                    cout << "zbdebug varDecl :"<<varDecl->getType()->getPointeeType().getAsString()<<endl;
//                }
//            }
            if (isa<ObjCPropertyDecl>(decl)) {
                ObjCPropertyDecl *propertyDecl = (ObjCPropertyDecl *)decl;
                string pSrcFile =  this->context->getSourceManager().getFilename(propertyDecl->getSourceRange().getBegin()).str();
                if (lastInterfaceDecl && (
                    !pSrcFile.compare(gFileName) || !pSrcFile.compare(gRightHeaderFileName))) {
                    if (propertyDecl->getType()->isAnyPointerType()) {
                        UnusedCodeUtil::setRelatedClsWithClass(propertyDecl->getType()->getPointeeType().getAsString(), lastInterfaceDecl->getNameAsString());
                    }
                }
            }
            return true;
        }
        
        bool VisitStmt(Stmt *s) {

            //[class selector]调用
            if(isa<ObjCMessageExpr>(s))
            {
                ObjCMessageExpr *objcExpr = (ObjCMessageExpr*)s;
                UnusedCodeUtil::appendCalleeClass((stringOfObjcMessage(objcExpr)));
//                ObjCMethodDecl
            }
            
            
            static bool nsstringfromclasscalled;
            if(isa<CallExpr>(s)) {
                CallExpr *callExpr = (CallExpr *)s;
                FunctionDecl *funcDecl = callExpr->getDirectCallee();
                if (funcDecl) {
                    //NSStringFromClass()调用
                    if (!funcDecl->getNameAsString().compare("NSClassFromString")) {
                            Expr *argExpr = callExpr->getArg(0);
                            bool invalid;
                            CharSourceRange argRange = CharSourceRange::getTokenRange(argExpr->getBeginLoc(), argExpr->getEndLoc());
                            StringRef str = Lexer::getSourceText(argRange, context->getSourceManager(), LangOptions(), &invalid);
                        
                        string className = string(str.str());
                        className = contentOfObjcString(className);
//                        UnusedCodeUtil::appendCalleeClass(className);

                    }
                    //只要某方法的声明在项目中，则加入改方法的引用
                    string funcSrcFile = context->getSourceManager().getFilename(funcDecl->getSourceRange().getBegin()).str();
                    if (funcSrcFile.length() && has_prefix(funcSrcFile, gSrcRootPath)) {
                        UnusedCodeUtil::appendCalleeClass(funcDecl->getNameAsString() + "()");
                    }
//                    cout << "CallExpr : "<<funcDecl->getNameAsString() + "()" <<" in file : " << funcSrcFile <<endl;
                }
            }
//            if (isa<ObjCStringLiteral>(s)) {
//                ObjCStringLiteral *sl = (ObjCStringLiteral *)s;
//
////                cout <<"ObjCStringLiteral:"<<sl->getString()->getString().str()<<endl;
//                if (nsstringfromclasscalled) {
//                    UnusedCodeUtil::appendCalleeClass(sl->getString()->getString().str());
//                    nsstringfromclasscalled = false;
//                    cout<<"NSClassFromString param: "<< sl->getString()->getString().str() <<endl;
//                }
//                cout<<"literal: "<<sl->getString()->getString().str()<<endl;
//            }
            return true;
        }
    };
    class UnusedCodeASTConsumer : public ASTConsumer
    {
    private:
        UnusedCodeASTVisitor visitor;
        void HandleTranslationUnit(ASTContext &context){
//            cout <<"zbdebug HandleTranslationUnit"<<endl;
            visitor.setContext(context);
            visitor.TraverseDecl(context.getTranslationUnitDecl());
            UnusedCodeUtil::synchronize();
            
        }
    };
    class UnsedCodeASTAction : public ASTFrontendAction
    {
    public:
        unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &Compiler,StringRef InFile){
//            cout << "lastFileName: "<<gFileName<<endl;
            gFileName = InFile.str();
            gRightHeaderFileName = gFileName.substr(0,gFileName.length() - 1) + "h";
//            cout << "newFileName: "<<gFileName<<endl;
            return unique_ptr<UnusedCodeASTConsumer>(new UnusedCodeASTConsumer);
        }
//        bool ParseArgs(const CompilerInstance &CI, const std::vector<std::string>& args){
//            size_t cnt = args.size();
//            if(cnt == 1){
//                string relativePath = args.at(0);
//                gSrcRootPath =  absolutePathFromRelative(relativePath);
//            }
//
//            return true;
//        }
    };
}

static llvm::cl::OptionCategory MyToolCategory("my-tool options");

int main(int argc, const char **argv) {
    auto ExpectedParser = CommonOptionsParser::create(argc, argv, MyToolCategory,llvm::cl::OneOrMore);
      if (!ExpectedParser) {
        // Fail gracefully for unsupported options.
        llvm::errs() << ExpectedParser.takeError();
        return 1;
      }
//    if(argc >= 1){
//        string relativePath = string(argv[0]);
//        gSrcRootPath =  absolutePathFromRelative(relativePath);
//    }
//    gSrcRootPath = string("/Users/zhangbo/AnalyzerDemo");
    gSrcRootPath = string("/tmp/call-graph");
    CommonOptionsParser& OptionsParser = ExpectedParser.get();
    ClangTool Tool(OptionsParser.getCompilations(),
                   OptionsParser.getSourcePathList());
    return Tool.run(newFrontendActionFactory<UnusedCodePlugin::UnsedCodeASTAction>().get());
}


