// Declares clang::SyntaxOnlyAction.
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include "clang/Sema/Sema.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"

// Declares llvm::cl::extrahelp.
#include "llvm/Support/CommandLine.h"

// Apply a custom category to all command-line options so that they are the
// only ones displayed.
static llvm::cl::OptionCategory tool_category("Constexpr-everything Options");

static llvm::cl::extrahelp common_help(
  clang::tooling::CommonOptionsParser::HelpMessage);

static llvm::cl::opt<bool> recurse_includes("recurse-includes");
static llvm::cl::opt<bool> diagnose_on_failure("diagnose-on-failure");

bool source_has_changed = false;

class constexprer_t : public clang::RecursiveASTVisitor<constexprer_t>
{
  clang::Rewriter rewriter_;
  clang::CompilerInstance& comp_inst_;

public:
  constexprer_t(clang::Rewriter rewriter, clang::CompilerInstance& comp_inst)
    : rewriter_(std::move(rewriter))
    , comp_inst_(comp_inst)
  {}

  // Set the method that gets called for each declaration node in the AST
  bool VisitDecl(clang::Decl* d_ptr)
  {
    // Referencing semantics engine
    auto& sema = comp_inst_.getSema();

    if (clang::isa<clang::FunctionDecl>(d_ptr)) {
      auto fd_ptr = clang::cast<clang::FunctionDecl>(d_ptr);

      if (!fd_ptr->hasBody() || fd_ptr->isMain())
        return true;

      if (!fd_ptr->isConstexpr()) {
        // Throwaway variable for isPotentialConstantExpr
        llvm::SmallVector<clang::PartialDiagnosticAt, 8> dgs_;
        if (sema.CheckConstexprFunctionDefinition(
              fd_ptr, clang::Sema::CheckConstexprKind::CheckValid) &&
            // NB: This check is currently *not*
            // performed by Sema with CheckValid
            clang::Expr::isPotentialConstantExpr(fd_ptr, dgs_) &&
            !fd_ptr->isMain()) {
          // Valid: rewrite
          rewriter_.InsertTextBefore(fd_ptr->getOuterLocStart(), "constexpr ");
          // Look for a previous declaration to rewrite
          auto prev = fd_ptr->getPreviousDecl();
          if (prev && !prev->isConstexpr())
            rewriter_.InsertTextBefore(prev->getOuterLocStart(), "constexpr ");
        } else if (diagnose_on_failure.getValue()) {
          // Otherwise: give diagnosis if asked to
          sema.CheckConstexprFunctionDefinition(
            fd_ptr, clang::Sema::CheckConstexprKind::Diagnose);
        }
      }
    }
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
        clang::Rewriter(compiler.getSourceManager(), compiler.getLangOpts()),
        compiler_)
  {}

  virtual bool HandleTopLevelDecl(clang::DeclGroupRef dg_ref)
  {
    auto const& sm = compiler_.getSourceManager();
    auto const mf_id = sm.getMainFileID();

    for (clang::Decl* decl_ptr : dg_ref) {
      auto decl_loc = decl_ptr->getLocation();
      if (sm.getFileID(decl_loc) == mf_id ||
          (recurse_includes.getValue() && !sm.isInSystemHeader(decl_loc)))
        constexprer_.TraverseDecl(decl_ptr);
    }
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
    llvm::outs() << in_file.str() << '\n';
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

  recurse_includes.addCategory(tool_category);
  recurse_includes.setDescription(
    "Updates includes recursively (unless they're system headers).");

  diagnose_on_failure.addCategory(tool_category);
  diagnose_on_failure.setDescription(
    "Emits diagnosis when a function doesn't meet constexpr requirements.");

  clt::CommonOptionsParser option_parser(argc, argv, tool_category);

  int res = 0;
  do {
    source_has_changed = false;
    clt::ClangTool tool(option_parser.getCompilations(),
                        option_parser.getSourcePathList());

    action_factory_t factory;

    res = tool.run(&factory);
  } while (source_has_changed);
  return res;
}
