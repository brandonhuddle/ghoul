#ifndef GULC_PARSER_HPP
#define GULC_PARSER_HPP

#include <ast/Decl.hpp>
#include <ast/Attr.hpp>
#include <vector>
#include <ast/decls/ImportDecl.hpp>
#include <ast/decls/VariableDecl.hpp>
#include <ast/exprs/IdentifierExpr.hpp>
#include <ast/exprs/ValueLiteralExpr.hpp>
#include <ast/stmts/BreakStmt.hpp>
#include <ast/stmts/CaseStmt.hpp>
#include <ast/stmts/CatchStmt.hpp>
#include <ast/stmts/CompoundStmt.hpp>
#include <ast/stmts/ContinueStmt.hpp>
#include <ast/stmts/DoStmt.hpp>
#include <ast/stmts/ForStmt.hpp>
#include <ast/stmts/GotoStmt.hpp>
#include <ast/stmts/IfStmt.hpp>
#include <ast/stmts/LabeledStmt.hpp>
#include <ast/stmts/ReturnStmt.hpp>
#include <ast/stmts/SwitchStmt.hpp>
#include <ast/stmts/TryStmt.hpp>
#include <ast/stmts/WhileStmt.hpp>
#include <ast/decls/FunctionDecl.hpp>
#include <ast/decls/NamespaceDecl.hpp>
#include <ast/decls/TemplateFunctionDecl.hpp>
#include <ast/Cont.hpp>
#include <ast/conts/RequiresCont.hpp>
#include <ast/conts/EnsuresCont.hpp>
#include <ast/conts/ThrowsCont.hpp>
#include <ast/decls/ConstructorDecl.hpp>
#include <ast/decls/DestructorDecl.hpp>
#include <ast/decls/StructDecl.hpp>
#include "Lexer.hpp"
#include "ASTFile.hpp"

namespace gulc {
    class Parser {
    public:
        ASTFile parseFile(unsigned int fileID, std::string const& filePath);

    private:
        unsigned int _fileID;
        std::string _filePath;
        gulc::Lexer _lexer;

        void printError(const std::string& errorMessage, TextPosition startPosition, TextPosition endPosition);
        void printWarning(const std::string& warningMessage, TextPosition startPosition, TextPosition endPosition);

        std::vector<Attr*> parseAttrs();
        Attr* parseAttr();

        /// E.g. `std.io`
        std::vector<Identifier> parseDotSeparatedIdentifiers();
        Identifier parseIdentifier();

        Type* parseType();

        Decl::Visibility parseDeclVisibility();

        Decl* parseDecl();
        ConstructorDecl* parseConstructorDecl(std::vector<Attr*> attributes, Decl::Visibility visibility,
                                              bool isConstExpr,
                                              DeclModifiers declModifiers, TextPosition startPosition);
        DestructorDecl* parseDestructorDecl(std::vector<Attr*> attributes, Decl::Visibility visibility,
                                            bool isConstExpr,
                                            DeclModifiers declModifiers, TextPosition startPosition);
        FunctionDecl* parseFunctionDecl(std::vector<Attr*> attributes, Decl::Visibility visibility, bool isConstExpr,
                                        DeclModifiers declModifiers, TextPosition startPosition);
        ImportDecl* parseImportDecl(std::vector<Attr*> attributes, TextPosition startPosition);
        NamespaceDecl* parseNamespaceDecl(std::vector<Attr*> attributes);
        std::vector<TemplateParameterDecl*> parseTemplateParameters();
        std::vector<ParameterDecl*> parseParameters(TextPosition* endPosition);
        StructDecl* parseStructDecl(std::vector<Attr*> attributes, Decl::Visibility visibility, bool isConstExpr,
                                    TextPosition startPosition, DeclModifiers declModifiers, bool isClass);
        VariableDecl* parseVariableDecl(std::vector<Attr*> attributes, Decl::Visibility visibility, bool isConstExpr,
                                        TextPosition startPosition, DeclModifiers declModifiers);

        std::vector<Cont*> parseConts();
        RequiresCont* parseRequiresCont();
        EnsuresCont* parseEnsuresCont();
        ThrowsCont* parseThrowsCont();

        Stmt* parseStmt();
        BreakStmt* parseBreakStmt();
        CaseStmt* parseCaseStmt();
        CatchStmt* parseCatchStmt();
        CompoundStmt* parseCompoundStmt();
        ContinueStmt* parseContinueStmt();
        DoStmt* parseDoStmt();
        ForStmt* parseForStmt();
        GotoStmt* parseGotoStmt();
        IfStmt* parseIfStmt();
        ReturnStmt* parseReturnStmt();
        SwitchStmt* parseSwitchStmt();
        TryStmt* parseTryStmt();
        WhileStmt* parseWhileStmt();

        Expr* parseVariableExpr();
        Expr* parseExpr();
        Expr* parseAssignment();
        Expr* parseTernary();
        Expr* parseLogicalOr();
        Expr* parseLogicalAnd();
        Expr* parseBitwiseOr();
        Expr* parseBitwiseXor();
        Expr* parseBitwiseAnd();
        Expr* parseEqualToNotEqualTo();
        Expr* parseGreaterThanLessThan();
        Expr* parseBitwiseShifts();
        Expr* parseAdditionSubtraction();
        Expr* parseMultiplicationDivisionOrRemainder();
        Expr* parseIsAsHas();
        Expr* parsePrefixes();
        Expr* parseCallPostfixOrMemberAccess();
        Expr* parseIdentifierOrLiteralExpr();
        IdentifierExpr* parseIdentifierExpr();
        ValueLiteralExpr* parseNumberLiteralExpr();
        ValueLiteralExpr* parseStringLiteralExpr();
        ValueLiteralExpr* parseCharacterLiteralExpr();
        Expr* parseArrayLiteralOrDimensionType();

    };
}

#endif //GULC_PARSER_HPP
