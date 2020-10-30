// Declares clang::SyntaxOnlyAction.
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"

// Declares llvm::cl::extrahelp.
#include "llvm/Support/CommandLine.h"

// Apply a custom category to all command-line options so that they are the
// only ones displayed.
static llvm::cl::OptionCategory MyToolCategory("my-tool options");

// CommonOptionsParser declares HelpMessage with a description of the common
// command-line options related to the compilation database and input files.
// It's nice to have this help message in all tools.
static llvm::cl::extrahelp CommonHelp(
  clang::tooling::CommonOptionsParser::HelpMessage);

// A help message for this specific tool can be added afterwards.
static llvm::cl::extrahelp MoreHelp("\nMore help text...\n");

class function_printer_t
  : public clang::ast_matchers::MatchFinder::MatchCallback
{
public:
  virtual void run(const clang::ast_matchers::MatchFinder::MatchResult& Result)
  {
    if (clang::FunctionDecl const* p =
          Result.Nodes.getNodeAs<clang::FunctionDecl>("f")) {
      p->dump();
    }
  }
};

// https://clang.llvm.org/docs/RAVFrontendAction.html
// https://eli.thegreenplace.net/2012/06/08/basic-source-to-source-transformation-with-clang/
// http://clang.llvm.org/docs/LibASTMatchersTutorial.html

class constexprer_t : public clang::RecursiveASTVisitor<constexprer_t>
{
public:
  bool TraverseDecl(clang::Decl* d_ptr)
  {
    d_ptr->dump();
    return true;
  }
};

class consumer_t : public clang::ASTConsumer
{
  constexprer_t constepxrer;

public:
  // Override the method that gets called for each parsed top-level
  // declaration.
  virtual bool HandleTopLevelDecl(clang::DeclGroupRef DR)
  {
    for (auto decl : DR)
      // Traverse the declaration using our AST visitor.
      constepxrer.TraverseDecl(decl);
    return true;
  }
};

class action_t : public clang::ASTFrontendAction
{
public:
  virtual std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(
    clang::CompilerInstance& Compiler,
    llvm::StringRef InFile)
  {
    return std::unique_ptr<clang::ASTConsumer>(new consumer_t);
  }
};

class action_factory_t: public clang::tooling::FrontendActionFactory
{
public:
  virtual std::unique_ptr<clang::FrontendAction> create()
  {
    return std::unique_ptr<clang::FrontendAction>(new action_t);
  }
};

int
main(int argc, const char** argv)
{
  namespace clt = clang::tooling;
  namespace clam = clang::ast_matchers;

  clt::CommonOptionsParser OptionsParser(argc, argv, MyToolCategory);
  clt::ClangTool Tool(OptionsParser.getCompilations(),
                      OptionsParser.getSourcePathList());

  action_factory_t factory;

  return Tool.run(&factory);
}
