// Declares clang::SyntaxOnlyAction.
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"

// Declares llvm::cl::extrahelp.
#include "llvm/Support/CommandLine.h"

//#include <iostream>
#include <fstream>
#include <string_view>

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

class constexprer_t : public clang::RecursiveASTVisitor<constexprer_t>
{
  clang::Rewriter& rewriter_;

public:
  constexprer_t(clang::Rewriter& rewriter)
    : rewriter_(rewriter)
  {}

  // Set the method that gets called for each declaration node in the AST
  bool VisitDecl(clang::Decl* d_ptr)
  {
    // Safety first
    if(!d_ptr)
      return true;

    // Is it a function declaration ?
    if (clang::isa<clang::FunctionDecl>(*d_ptr))
      // Not constexpr yet ?
      if (auto fd_ptr = clang::cast<clang::FunctionDecl>(d_ptr); !fd_ptr->isConstexpr())
        // Rewrite
        rewriter_.InsertTextBefore(fd_ptr->getLocation(), "constexpr ");

    // Next one, please
    return true;
  }
};

class consumer_t : public clang::ASTConsumer
{
  // Storing compiler instance and file info
  clang::CompilerInstance& compiler_;
  llvm::StringRef in_file_;

  // Attach a rewriter to the ASTConsumer
  clang::Rewriter rewriter_;

  // This is our visitor
  constexprer_t constexprer_;

public:
  consumer_t(clang::CompilerInstance& compiler, llvm::StringRef in_file)
    : compiler_(compiler)
    , in_file_(in_file)
    , rewriter_(compiler.getSourceManager(), compiler.getLangOpts())
    , constexprer_(rewriter_)
  {}

  // Override the method that gets called for each parsed top-level
  // declaration.
  virtual bool HandleTopLevelDecl(clang::DeclGroupRef dgr)
  {
    auto const& sm = compiler_.getSourceManager();
    auto const mf_id = sm.getMainFileID();

    for (clang::Decl* decl_ptr : dgr)
      // Only watch the current file
      if (sm.getFileID(decl_ptr->getLocation()) == mf_id)
        constexprer_.TraverseDecl(decl_ptr);

    return true;
  }

  ~consumer_t()
  {
    auto r_buf =
      rewriter_.getRewriteBufferFor(rewriter_.getSourceMgr().getMainFileID());
    std::ofstream(in_file_.str(), std::ios::trunc) << std::string(r_buf->begin(), r_buf->end()) << '\n';
  }
};

class action_t : public clang::ASTFrontendAction
{
public:
  virtual std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(
    clang::CompilerInstance& compiler,
    llvm::StringRef in_file)
  {
    return std::unique_ptr<clang::ASTConsumer>(
      new consumer_t(compiler, in_file));
  }
};

class action_factory_t : public clang::tooling::FrontendActionFactory
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

  clt::CommonOptionsParser OptionsParser(argc, argv, MyToolCategory);
  clt::ClangTool Tool(OptionsParser.getCompilations(),
                      OptionsParser.getSourcePathList());

  action_factory_t factory;

  return Tool.run(&factory);
}
