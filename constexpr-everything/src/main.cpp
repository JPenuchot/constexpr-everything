// Declares clang::SyntaxOnlyAction.
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"

// Declares llvm::cl::extrahelp.
#include "llvm/Support/CommandLine.h"

// Apply a custom category to all command-line options so that they are the
// only ones displayed.
static llvm::cl::OptionCategory tool_category("Constexpr-everything Options");
static llvm::cl::extrahelp common_help(
  clang::tooling::CommonOptionsParser::HelpMessage);
static llvm::cl::extrahelp more_help("\nMake everything constexpr.\n");

class constexprer_t : public clang::RecursiveASTVisitor<constexprer_t>
{
  clang::Rewriter rewriter_;

public:
  constexprer_t(clang::Rewriter rewriter)
    : rewriter_(std::move(rewriter))
  {}

  // Set the method that gets called for each declaration node in the AST
  bool VisitDecl(clang::Decl* d_ptr)
  {
    // Safety first
    if (!d_ptr)
      return true;

    // Is it a function declaration ?
    if (clang::isa<clang::FunctionDecl>(*d_ptr)) {
      auto fd_ptr = clang::cast<clang::FunctionDecl>(d_ptr);
      // Is it not constexpr yet ?
      if (!fd_ptr->isConstexpr())
        // Rewrite
        rewriter_.InsertTextBefore(fd_ptr->getOuterLocStart(), "constexpr ");
    }

    // Proceed
    return true;
  }

  ~constexprer_t() { rewriter_.overwriteChangedFiles(); }
};

class consumer_t : public clang::ASTConsumer
{
  clang::CompilerInstance& compiler_;
  llvm::StringRef in_file_;
  constexprer_t constexprer_;

public:
  consumer_t(clang::CompilerInstance& compiler, llvm::StringRef in_file)
    : compiler_(compiler)
    , in_file_(in_file)
    , constexprer_(
        clang::Rewriter(compiler.getSourceManager(), compiler.getLangOpts()))
  {}

  // Override the method that gets called for each parsed top-level
  // declaration.
  virtual bool HandleTopLevelDecl(clang::DeclGroupRef dg_ref)
  {
    auto const& sm = compiler_.getSourceManager();
    auto const mf_id = sm.getMainFileID();

    for (clang::Decl* decl_ptr : dg_ref)
      // Only watch the current file
      if (sm.getFileID(decl_ptr->getLocation()) == mf_id)
        constexprer_.TraverseDecl(decl_ptr);

    return true;
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

  clt::CommonOptionsParser option_parser(argc, argv, tool_category);
  clt::ClangTool tool(option_parser.getCompilations(),
                      option_parser.getSourcePathList());

  action_factory_t factory;

  return tool.run(&factory);
}
