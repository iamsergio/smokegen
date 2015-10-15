#ifndef SMOKEGEN_ASTVISITOR
#define SMOKEGEN_ASTVISITOR

#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Sema/Sema.h>

#include "type.h"

class SmokegenASTVisitor : public clang::RecursiveASTVisitor<SmokegenASTVisitor> {
public:
    SmokegenASTVisitor(clang::CompilerInstance &ci) : ci(ci) {}

    bool VisitCXXRecordDecl(clang::CXXRecordDecl *D);
    bool VisitEnumDecl(clang::EnumDecl *D);
    bool VisitNamespaceDecl(clang::NamespaceDecl *D);
    bool VisitFunctionDecl(clang::FunctionDecl *D);

private:
    clang::PrintingPolicy pp() const { return ci.getSema().getPrintingPolicy(); }

    clang::QualType getReturnTypeForMethod(const clang::CXXMethodDecl* method) const;

    Class* registerClass(const clang::CXXRecordDecl* clangClass) const;
    Type* registerType(const clang::QualType clangType) const;

    Access toAccess(clang::AccessSpecifier clangAccess) const;

    clang::CompilerInstance &ci;
};

#endif