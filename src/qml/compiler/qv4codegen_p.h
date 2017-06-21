/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/
#ifndef QV4CODEGEN_P_H
#define QV4CODEGEN_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "private/qv4global_p.h"
#include "qv4jsir_p.h"
#include <private/qqmljsastvisitor_p.h>
#include <private/qqmljsast_p.h>
#include <private/qqmljsengine_p.h>
#include <private/qv4instr_moth_p.h>
#include <QtCore/QStringList>
#include <QStack>
#ifndef V4_BOOTSTRAP
#include <qqmlerror.h>
#endif
#include <private/qv4util_p.h>
#include <private/qv4bytecodegenerator_p.h>

QT_BEGIN_NAMESPACE

namespace QV4 {
struct ControlFlow;
struct ControlFlowCatch;
struct ControlFlowFinally;

namespace Compiler {
struct JSUnitGenerator;
}
namespace Moth {
struct Instruction;
}
}

namespace QQmlJS {
namespace AST {
class UiParameterList;
}

class Q_QML_PRIVATE_EXPORT Codegen: protected AST::Visitor
{
protected:
    using BytecodeGenerator = QV4::Moth::BytecodeGenerator;
    using Instruction = QV4::Moth::Instruction;
public:
    Codegen(QV4::Compiler::JSUnitGenerator *jsUnitGenerator, bool strict);

    enum CompilationMode {
        GlobalCode,
        EvalCode,
        FunctionCode,
        QmlBinding // This is almost the same as EvalCode, except:
                   //  * function declarations are moved to the return address when encountered
                   //  * return statements are allowed everywhere (like in FunctionCode)
                   //  * variable declarations are treated as true locals (like in FunctionCode)
    };

    void generateFromProgram(const QString &fileName,
                             const QString &sourceCode,
                             AST::Program *ast,
                             QV4::IR::Module *module,
                             CompilationMode mode = GlobalCode,
                             const QStringList &inheritedLocals = QStringList());
    void generateFromFunctionExpression(const QString &fileName,
                             const QString &sourceCode,
                             AST::FunctionExpression *ast,
                             QV4::IR::Module *module);

public:
    struct Reference {
        enum Type {
            Invalid,
            Temp,
            Local,
            Argument,
            Name,
            Member,
            Subscript,
            Closure,
            QmlScopeObject,
            QmlContextObject,
            LastLValue = QmlContextObject,
            Const,
            This
        } type = Invalid;

        bool isLValue() const { return type <= LastLValue; }

        Reference(Codegen *cg, Type type = Invalid) : type(type), codegen(cg) {}
        Reference()
            : type(Invalid)
            , codegen(nullptr)
        {}
        Reference(const Reference &other);
        ~Reference();

        Reference &operator =(const Reference &other);

        bool operator==(const Reference &other) const;

        bool isValid() const { return type != Invalid; }
        bool isTempLocalArg() const { return isValid() && type < Argument; }
        bool isConst() const { return type == Const; }

        static Reference fromTemp(Codegen *cg, int tempIndex = -1) {
            Reference r(cg, Temp);
            if (tempIndex == -1)
                tempIndex = cg->bytecodeGenerator->newTemp();
            r.base = QV4::Moth::Param::createTemp(tempIndex);
            return r;
        }
        static Reference fromLocal(Codegen *cg, uint index, uint scope) {
            Reference r(cg, Local);
            r.base = QV4::Moth::Param::createScopedLocal(index, scope);
            return r;
        }
        static Reference fromArgument(Codegen *cg, uint index, uint scope) {
            Reference r(cg, Argument);
            r.base = QV4::Moth::Param::createArgument(index, scope);
            return r;
        }
        static Reference fromName(Codegen *cg, const QString &name) {
            Reference r(cg, Name);
            r.nameIndex = cg->registerString(name);
            return r;
        }
        static Reference fromMember(const Reference &baseRef, const QString &name) {
            Reference r(baseRef.codegen, Member);
            r.base = baseRef.asRValue();
            r.nameIndex = r.codegen->registerString(name);
            return r;
        }
        static Reference fromSubscript(const Reference &baseRef, const Reference &subscript) {
            Reference r(baseRef.codegen, Subscript);
            r.base = baseRef.asRValue();
            r.subscript = subscript.asRValue();
            return r;
        }
        static Reference fromConst(Codegen *cg, QV4::ReturnedValue constant) {
            Reference r(cg, Const);
            r.constant = constant;
            return r;
        }
        static Reference fromClosure(Codegen *cg, int functionId) {
            Reference r(cg, Closure);
            r.closureId = functionId;
            return r;
        }
        static Reference fromQmlScopeObject(const Reference &base, int index) {
            Reference r(base.codegen, QmlScopeObject);
            r.base = base.asRValue();
            r.qmlIndex = index;
            return r;
        }
        static Reference fromQmlContextObject(const Reference &base, int index) {
            Reference r(base.codegen, QmlContextObject);
            r.base = base.asRValue();
            r.qmlIndex = index;
            return r;
        }
        static Reference fromThis(Codegen *cg) {
            Reference r(cg, This);
            return r;
        }

        bool isSimple() const {
            switch (type) {
            case Temp:
            case Local:
            case Argument:
            case Const:
                return true;
            default:
                return false;
            }
        }

        void store(const Reference &r) const;
        void storeConsume(Reference &r) const;

        QV4::Moth::Param asRValue() const;
        QV4::Moth::Param asLValue() const;

        void writeBack() const;
        void load(uint temp) const;

        QV4::Moth::Param base;
        union {
            uint nameIndex;
            QV4::Moth::Param subscript;
            QV4::ReturnedValue constant;
            int closureId;
            int qmlIndex;
        };
        mutable int tempIndex = -1;
        mutable bool needsWriteBack = false;
        mutable bool isArgOrEval = false;
        Codegen *codegen;

    };

    struct TempScope {
        TempScope(Codegen *cg)
            : function(cg->_function),
              tempCountForScope(function->currentTemp) {}
        TempScope(QV4::IR::Function *f)
            : function(f),
              tempCountForScope(f->currentTemp) {}
        ~TempScope() {
            function->currentTemp = tempCountForScope;
        }
        QV4::IR::Function *function;
        int tempCountForScope;
    };

    struct ObjectPropertyValue {
        ObjectPropertyValue()
            : getter(-1)
            , setter(-1)
            , keyAsIndex(UINT_MAX)
        {}

        Reference rvalue;
        int getter; // index in _module->functions or -1 if not set
        int setter;
        uint keyAsIndex;

        bool hasGetter() const { return getter >= 0; }
        bool hasSetter() const { return setter >= 0; }
    };
protected:

    enum Format { ex, cx, nx };
    struct Result {
        Reference result;

        QV4::IR::Expr *code;
        const BytecodeGenerator::Label *iftrue;
        const BytecodeGenerator::Label *iffalse;
        Format format;
        Format requested;
        bool trueBlockFollowsCondition = false;

        Result(const Reference &lrvalue)
            : result(lrvalue)
            , code(nullptr)
            , iftrue(nullptr)
            , iffalse(nullptr)
            , format(ex)
            , requested(ex)
        {
        }

        explicit Result(Format requested = ex)
            : code(0)
            , iftrue(0)
            , iffalse(0)
            , format(ex)
            , requested(requested) {}

        explicit Result(const BytecodeGenerator::Label *iftrue,
                        const BytecodeGenerator::Label *iffalse,
                        bool trueBlockFollowsCondition)
            : code(0)
            , iftrue(iftrue)
            , iffalse(iffalse)
            , format(ex)
            , requested(cx)
            , trueBlockFollowsCondition(trueBlockFollowsCondition)
        {}

        inline QV4::IR::Expr *operator*() const { Q_ASSERT(format == ex); return code; }
        inline QV4::IR::Expr *operator->() const { Q_ASSERT(format == ex); return code; }

        bool accept(Format f)
        {
            if (requested == f) {
                format = f;
                return true;
            }
            return false;
        }
    };

    struct Environment {
        Environment *parent;

        enum MemberType {
            UndefinedMember,
            VariableDefinition,
            VariableDeclaration,
            FunctionDefinition
        };

        struct Member {
            MemberType type;
            int index;
            AST::FunctionExpression *function;
            AST::VariableDeclaration::VariableScope scope;

            bool isLexicallyScoped() const { return this->scope != AST::VariableDeclaration::FunctionScope; }
        };
        typedef QMap<QString, Member> MemberMap;

        MemberMap members;
        AST::FormalParameterList *formals;
        int maxNumberOfArguments;
        bool hasDirectEval;
        bool hasNestedFunctions;
        bool isStrict;
        bool isNamedFunctionExpression;
        bool usesThis;
        enum UsesArgumentsObject {
            ArgumentsObjectUnknown,
            ArgumentsObjectNotUsed,
            ArgumentsObjectUsed
        };

        UsesArgumentsObject usesArgumentsObject;

        CompilationMode compilationMode;

        Environment(Environment *parent, CompilationMode mode)
            : parent(parent)
            , formals(0)
            , maxNumberOfArguments(0)
            , hasDirectEval(false)
            , hasNestedFunctions(false)
            , isStrict(false)
            , isNamedFunctionExpression(false)
            , usesThis(false)
            , usesArgumentsObject(ArgumentsObjectUnknown)
            , compilationMode(mode)
        {
            if (parent && parent->isStrict)
                isStrict = true;
        }

        int findMember(const QString &name) const
        {
            MemberMap::const_iterator it = members.find(name);
            if (it == members.end())
                return -1;
            Q_ASSERT((*it).index != -1 || !parent);
            return (*it).index;
        }

        bool memberInfo(const QString &name, const Member **m) const
        {
            Q_ASSERT(m);
            MemberMap::const_iterator it = members.find(name);
            if (it == members.end()) {
                *m = 0;
                return false;
            }
            *m = &(*it);
            return true;
        }

        bool lookupMember(const QString &name, Environment **scope, int *index, int *distance)
        {
            Environment *it = this;
            *distance = 0;
            for (; it; it = it->parent, ++(*distance)) {
                int idx = it->findMember(name);
                if (idx != -1) {
                    *scope = it;
                    *index = idx;
                    return true;
                }
            }
            return false;
        }

        void enter(const QString &name, MemberType type, AST::VariableDeclaration::VariableScope scope, AST::FunctionExpression *function = 0)
        {
            if (! name.isEmpty()) {
                if (type != FunctionDefinition) {
                    for (AST::FormalParameterList *it = formals; it; it = it->next)
                        if (it->name == name)
                            return;
                }
                MemberMap::iterator it = members.find(name);
                if (it == members.end()) {
                    Member m;
                    m.index = -1;
                    m.type = type;
                    m.function = function;
                    m.scope = scope;
                    members.insert(name, m);
                } else {
                    Q_ASSERT(scope == (*it).scope);
                    if ((*it).type <= type) {
                        (*it).type = type;
                        (*it).function = function;
                    }
                }
            }
        }
    };

    Environment *newEnvironment(AST::Node *node, Environment *parent, CompilationMode compilationMode)
    {
        Environment *env = new Environment(parent, compilationMode);
        _envMap.insert(node, env);
        return env;
    }

    struct UiMember {
    };

    void enterEnvironment(AST::Node *node);
    void leaveEnvironment();

    void leaveLoop();

    enum UnaryOperation {
        UPlus,
        UMinus,
        PreIncrement,
        PreDecrement,
        PostIncrement,
        PostDecrement,
        Not,
        Compl
    };

    Reference unop(UnaryOperation op, const Reference &expr);

    int registerString(const QString &name) {
        return jsUnitGenerator->registerString(name);
    }

    // Returns index in _module->functions
    int defineFunction(const QString &name, AST::Node *ast,
                       AST::FormalParameterList *formals,
                       AST::SourceElements *body,
                       const QStringList &inheritedLocals = QStringList());

    void statement(AST::Statement *ast);
    void statement(AST::ExpressionNode *ast);
    void condition(AST::ExpressionNode *ast, const BytecodeGenerator::Label *iftrue,
                   const BytecodeGenerator::Label *iffalse,
                   bool trueBlockFollowsCondition);
    Reference expression(AST::ExpressionNode *ast);
    Result sourceElement(AST::SourceElement *ast);
    UiMember uiObjectMember(AST::UiObjectMember *ast);

    void accept(AST::Node *node);

    void functionBody(AST::FunctionBody *ast);
    void program(AST::Program *ast);
    void sourceElements(AST::SourceElements *ast);
    void variableDeclaration(AST::VariableDeclaration *ast);
    void variableDeclarationList(AST::VariableDeclarationList *ast);

    Reference referenceForName(const QString &name, bool lhs);

    // Hook provided to implement QML lookup semantics
    virtual Reference fallbackNameLookup(const QString &name);
    virtual void beginFunctionBodyHook() {}

    // nodes
    bool visit(AST::ArgumentList *ast) override;
    bool visit(AST::CaseBlock *ast) override;
    bool visit(AST::CaseClause *ast) override;
    bool visit(AST::CaseClauses *ast) override;
    bool visit(AST::Catch *ast) override;
    bool visit(AST::DefaultClause *ast) override;
    bool visit(AST::ElementList *ast) override;
    bool visit(AST::Elision *ast) override;
    bool visit(AST::Finally *ast) override;
    bool visit(AST::FormalParameterList *ast) override;
    bool visit(AST::FunctionBody *ast) override;
    bool visit(AST::Program *ast) override;
    bool visit(AST::PropertyNameAndValue *ast) override;
    bool visit(AST::PropertyAssignmentList *ast) override;
    bool visit(AST::PropertyGetterSetter *ast) override;
    bool visit(AST::SourceElements *ast) override;
    bool visit(AST::StatementList *ast) override;
    bool visit(AST::UiArrayMemberList *ast) override;
    bool visit(AST::UiImport *ast) override;
    bool visit(AST::UiHeaderItemList *ast) override;
    bool visit(AST::UiPragma *ast) override;
    bool visit(AST::UiObjectInitializer *ast) override;
    bool visit(AST::UiObjectMemberList *ast) override;
    bool visit(AST::UiParameterList *ast) override;
    bool visit(AST::UiProgram *ast) override;
    bool visit(AST::UiQualifiedId *ast) override;
    bool visit(AST::UiQualifiedPragmaId *ast) override;
    bool visit(AST::VariableDeclaration *ast) override;
    bool visit(AST::VariableDeclarationList *ast) override;

    // expressions
    bool visit(AST::Expression *ast) override;
    bool visit(AST::ArrayLiteral *ast) override;
    bool visit(AST::ArrayMemberExpression *ast) override;
    bool visit(AST::BinaryExpression *ast) override;
    bool visit(AST::CallExpression *ast) override;
    bool visit(AST::ConditionalExpression *ast) override;
    bool visit(AST::DeleteExpression *ast) override;
    bool visit(AST::FalseLiteral *ast) override;
    bool visit(AST::FieldMemberExpression *ast) override;
    bool visit(AST::FunctionExpression *ast) override;
    bool visit(AST::IdentifierExpression *ast) override;
    bool visit(AST::NestedExpression *ast) override;
    bool visit(AST::NewExpression *ast) override;
    bool visit(AST::NewMemberExpression *ast) override;
    bool visit(AST::NotExpression *ast) override;
    bool visit(AST::NullExpression *ast) override;
    bool visit(AST::NumericLiteral *ast) override;
    bool visit(AST::ObjectLiteral *ast) override;
    bool visit(AST::PostDecrementExpression *ast) override;
    bool visit(AST::PostIncrementExpression *ast) override;
    bool visit(AST::PreDecrementExpression *ast) override;
    bool visit(AST::PreIncrementExpression *ast) override;
    bool visit(AST::RegExpLiteral *ast) override;
    bool visit(AST::StringLiteral *ast) override;
    bool visit(AST::ThisExpression *ast) override;
    bool visit(AST::TildeExpression *ast) override;
    bool visit(AST::TrueLiteral *ast) override;
    bool visit(AST::TypeOfExpression *ast) override;
    bool visit(AST::UnaryMinusExpression *ast) override;
    bool visit(AST::UnaryPlusExpression *ast) override;
    bool visit(AST::VoidExpression *ast) override;
    bool visit(AST::FunctionDeclaration *ast) override;

    // source elements
    bool visit(AST::FunctionSourceElement *ast) override;
    bool visit(AST::StatementSourceElement *ast) override;

    // statements
    bool visit(AST::Block *ast) override;
    bool visit(AST::BreakStatement *ast) override;
    bool visit(AST::ContinueStatement *ast) override;
    bool visit(AST::DebuggerStatement *ast) override;
    bool visit(AST::DoWhileStatement *ast) override;
    bool visit(AST::EmptyStatement *ast) override;
    bool visit(AST::ExpressionStatement *ast) override;
    bool visit(AST::ForEachStatement *ast) override;
    bool visit(AST::ForStatement *ast) override;
    bool visit(AST::IfStatement *ast) override;
    bool visit(AST::LabelledStatement *ast) override;
    bool visit(AST::LocalForEachStatement *ast) override;
    bool visit(AST::LocalForStatement *ast) override;
    bool visit(AST::ReturnStatement *ast) override;
    bool visit(AST::SwitchStatement *ast) override;
    bool visit(AST::ThrowStatement *ast) override;
    bool visit(AST::TryStatement *ast) override;
    bool visit(AST::VariableStatement *ast) override;
    bool visit(AST::WhileStatement *ast) override;
    bool visit(AST::WithStatement *ast) override;

    // ui object members
    bool visit(AST::UiArrayBinding *ast) override;
    bool visit(AST::UiObjectBinding *ast) override;
    bool visit(AST::UiObjectDefinition *ast) override;
    bool visit(AST::UiPublicMember *ast) override;
    bool visit(AST::UiScriptBinding *ast) override;
    bool visit(AST::UiSourceElement *ast) override;

    bool throwSyntaxErrorOnEvalOrArgumentsInStrictMode(const Reference &r, const AST::SourceLocation &loc);
    virtual void throwSyntaxError(const AST::SourceLocation &loc, const QString &detail);
    virtual void throwReferenceError(const AST::SourceLocation &loc, const QString &detail);

public:
    QList<DiagnosticMessage> errors() const;
#ifndef V4_BOOTSTRAP
    QList<QQmlError> qmlErrors() const;
#endif

    QV4::Moth::Param binopHelper(QSOperator::Op oper, const QV4::Moth::Param &left,
                                 const QV4::Moth::Param &right, const QV4::Moth::Param &dest);
    int pushArgs(AST::ArgumentList *args);

    void handleTryCatch(AST::TryStatement *ast);
    void handleTryFinally(AST::TryStatement *ast);
protected:
    friend struct QV4::ControlFlow;
    friend struct QV4::ControlFlowCatch;
    friend struct QV4::ControlFlowFinally;
    Result _expr;
    QString _property;
    UiMember _uiMember;
    QV4::IR::Module *_module;
    QV4::IR::Function *_function;
    QV4::IR::BasicBlock *_block;
    BytecodeGenerator::Label _exitBlock;
    unsigned _returnAddress;
    Environment *_variableEnvironment;
    QV4::ControlFlow *_controlFlow;
    AST::LabelledStatement *_labelledStatement;
    QHash<AST::Node *, Environment *> _envMap;
    QHash<AST::FunctionExpression *, int> _functionMap;
    QV4::Compiler::JSUnitGenerator *jsUnitGenerator;
    BytecodeGenerator *bytecodeGenerator = 0;
    bool _strictMode;

    bool _fileNameIsUrl;
    bool hasError;
    QList<QQmlJS::DiagnosticMessage> _errors;


    class ScanFunctions: protected Visitor
    {
        typedef QV4::TemporaryAssignment<bool> TemporaryBoolAssignment;
    public:
        ScanFunctions(Codegen *cg, const QString &sourceCode, CompilationMode defaultProgramMode);
        void operator()(AST::Node *node);

        void enterEnvironment(AST::Node *node, CompilationMode compilationMode);
        void leaveEnvironment();

        void enterQmlScope(AST::Node *ast, const QString &name)
        { enterFunction(ast, name, /*formals*/0, /*body*/0, /*expr*/0, /*isExpression*/false); }

        void enterQmlFunction(AST::FunctionDeclaration *ast)
        { enterFunction(ast, false, false); }

    protected:
        using Visitor::visit;
        using Visitor::endVisit;

        void checkDirectivePrologue(AST::SourceElements *ast);

        void checkName(const QStringRef &name, const AST::SourceLocation &loc);
        void checkForArguments(AST::FormalParameterList *parameters);

        bool visit(AST::Program *ast) override;
        void endVisit(AST::Program *) override;

        bool visit(AST::CallExpression *ast) override;
        bool visit(AST::NewMemberExpression *ast) override;
        bool visit(AST::ArrayLiteral *ast) override;
        bool visit(AST::VariableDeclaration *ast) override;
        bool visit(AST::IdentifierExpression *ast) override;
        bool visit(AST::ExpressionStatement *ast) override;
        bool visit(AST::FunctionExpression *ast) override;

        void enterFunction(AST::FunctionExpression *ast, bool enterName, bool isExpression = true);

        void endVisit(AST::FunctionExpression *) override;

        bool visit(AST::ObjectLiteral *ast) override;

        bool visit(AST::PropertyGetterSetter *ast) override;
        void endVisit(AST::PropertyGetterSetter *) override;

        bool visit(AST::FunctionDeclaration *ast) override;
        void endVisit(AST::FunctionDeclaration *) override;

        bool visit(AST::WithStatement *ast) override;

        bool visit(AST::DoWhileStatement *ast) override;
        bool visit(AST::ForStatement *ast) override;
        bool visit(AST::LocalForStatement *ast) override;
        bool visit(AST::ForEachStatement *ast) override;
        bool visit(AST::LocalForEachStatement *ast) override;
        bool visit(AST::ThisExpression *ast) override;

        bool visit(AST::Block *ast) override;

    protected:
        void enterFunction(AST::Node *ast, const QString &name, AST::FormalParameterList *formals, AST::FunctionBody *body, AST::FunctionExpression *expr, bool isExpression);

    // fields:
        Codegen *_cg;
        const QString _sourceCode;
        Environment *_variableEnvironment;
        QStack<Environment *> _envStack;

        bool _allowFuncDecls;
        CompilationMode defaultProgramMode;
    };

};

#ifndef V4_BOOTSTRAP
class RuntimeCodegen : public Codegen
{
public:
    RuntimeCodegen(QV4::ExecutionEngine *engine, QV4::Compiler::JSUnitGenerator *jsUnitGenerator, bool strict)
        : Codegen(jsUnitGenerator, strict)
        , engine(engine)
    {}

    void throwSyntaxError(const AST::SourceLocation &loc, const QString &detail) override;
    void throwReferenceError(const AST::SourceLocation &loc, const QString &detail) override;
private:
    QV4::ExecutionEngine *engine;
};
#endif // V4_BOOTSTRAP

}

QT_END_NAMESPACE

#endif // QV4CODEGEN_P_H
