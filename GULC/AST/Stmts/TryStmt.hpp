#ifndef GULC_TRYSTMT_HPP
#define GULC_TRYSTMT_HPP

#include <AST/Stmt.hpp>
#include <vector>
#include "TryCatchStmt.hpp"
#include "TryFinallyStmt.hpp"
#include "CompoundStmt.hpp"

namespace gulc {
    class TryStmt : public Stmt {
    public:
        static bool classof(const Stmt *stmt) { return stmt->getStmtKind() == StmtKind::Try; }

        TryStmt(TextPosition startPosition, TextPosition endPosition,
                CompoundStmt* encapsulatedStmt, std::vector<TryCatchStmt*> catchStmts, TryFinallyStmt* finallyStmt)
                : Stmt(StmtKind::Try, startPosition, endPosition),
                  _encapsulatedStmt(encapsulatedStmt), _catchStmts(std::move(catchStmts)), _finallyStmt(finallyStmt) {}

        const CompoundStmt* encapsulatedStmt() const { return _encapsulatedStmt; }
        const std::vector<TryCatchStmt*>& catchStmts() const { return _catchStmts; }
        const TryFinallyStmt* finallyStmt() const { return _finallyStmt; }

        bool hasCatchStmts() const { return !_catchStmts.empty(); }
        bool hasFinallyStmt() const { return _finallyStmt != nullptr; }

        ~TryStmt() override {
            for (TryCatchStmt* tryCatchStmt : _catchStmts) {
                delete tryCatchStmt;
            }

            delete _encapsulatedStmt;
            delete _finallyStmt;
        }

    private:
        // We require 'CompoundStmt' for 'try', 'catch', and 'finally' to prevent unwanted errors for:
        //
        //     try
        //         // blah...
        //     catch (Exception2 e2)
        //     catch (Exception e)
        //         try
        //             // blah again, use imagination...
        //         catch (IOException ioe)
        //             // blah...
        //         catch (Exception3 e3)
        //
        // In situations similar to that it would be difficult to tell if the nested catch statements are nested
        // or attached to the first try statement
        CompoundStmt* _encapsulatedStmt;
        std::vector<TryCatchStmt*> _catchStmts;
        TryFinallyStmt* _finallyStmt;

    };
}

#endif //GULC_TRYSTMT_HPP