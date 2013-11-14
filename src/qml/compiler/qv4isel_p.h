/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QV4ISEL_P_H
#define QV4ISEL_P_H

#include "private/qv4global_p.h"
#include "qv4jsir_p.h"
#include <private/qv4compileddata_p.h>
#include <private/qv4compiler_p.h>

#include <qglobal.h>
#include <QHash>

QT_BEGIN_NAMESPACE

class QQmlEnginePrivate;

namespace QV4 {
class ExecutableAllocator;
struct Function;
}

namespace QQmlJS {

class Q_QML_EXPORT EvalInstructionSelection
{
public:
    EvalInstructionSelection(QV4::ExecutableAllocator *execAllocator, V4IR::Module *module, QV4::Compiler::JSUnitGenerator *jsGenerator);
    virtual ~EvalInstructionSelection() = 0;

    QV4::CompiledData::CompilationUnit *compile(bool generateUnitData = true);

    void setUseFastLookups(bool b) { useFastLookups = b; }

    int registerString(const QString &str) { return jsGenerator->registerString(str); }
    uint registerGetterLookup(const QString &name) { return jsGenerator->registerGetterLookup(name); }
    uint registerSetterLookup(const QString &name) { return jsGenerator->registerSetterLookup(name); }
    uint registerGlobalGetterLookup(const QString &name) { return jsGenerator->registerGlobalGetterLookup(name); }
    int registerRegExp(QQmlJS::V4IR::RegExp *regexp) { return jsGenerator->registerRegExp(regexp); }
    int registerJSClass(QQmlJS::V4IR::ExprList *args) { return jsGenerator->registerJSClass(args); }
    QV4::Compiler::JSUnitGenerator *jsUnitGenerator() const { return jsGenerator; }

protected:
    virtual void run(int functionIndex) = 0;
    virtual QV4::CompiledData::CompilationUnit *backendCompileStep() = 0;

    bool useFastLookups;
    QV4::ExecutableAllocator *executableAllocator;
    QV4::Compiler::JSUnitGenerator *jsGenerator;
    QScopedPointer<QV4::Compiler::JSUnitGenerator> ownJSGenerator;
    V4IR::Module *irModule;
};

class Q_QML_EXPORT EvalISelFactory
{
public:
    virtual ~EvalISelFactory() = 0;
    virtual EvalInstructionSelection *create(QQmlEnginePrivate *qmlEngine, QV4::ExecutableAllocator *execAllocator, V4IR::Module *module, QV4::Compiler::JSUnitGenerator *jsGenerator) = 0;
    virtual bool jitCompileRegexps() const = 0;
};

namespace V4IR {
class Q_QML_EXPORT IRDecoder: protected V4IR::StmtVisitor
{
public:
    IRDecoder() : _function(0) {}
    virtual ~IRDecoder() = 0;

    virtual void visitPhi(V4IR::Phi *) {}

public: // visitor methods for StmtVisitor:
    virtual void visitMove(V4IR::Move *s);
    virtual void visitExp(V4IR::Exp *s);

public: // to implement by subclasses:
    virtual void callBuiltinInvalid(V4IR::Name *func, V4IR::ExprList *args, V4IR::Temp *result) = 0;
    virtual void callBuiltinTypeofMember(V4IR::Expr *base, const QString &name, V4IR::Temp *result) = 0;
    virtual void callBuiltinTypeofSubscript(V4IR::Expr *base, V4IR::Expr *index, V4IR::Temp *result) = 0;
    virtual void callBuiltinTypeofName(const QString &name, V4IR::Temp *result) = 0;
    virtual void callBuiltinTypeofValue(V4IR::Expr *value, V4IR::Temp *result) = 0;
    virtual void callBuiltinDeleteMember(V4IR::Temp *base, const QString &name, V4IR::Temp *result) = 0;
    virtual void callBuiltinDeleteSubscript(V4IR::Temp *base, V4IR::Expr *index, V4IR::Temp *result) = 0;
    virtual void callBuiltinDeleteName(const QString &name, V4IR::Temp *result) = 0;
    virtual void callBuiltinDeleteValue(V4IR::Temp *result) = 0;
    virtual void callBuiltinThrow(V4IR::Expr *arg) = 0;
    virtual void callBuiltinReThrow() = 0;
    virtual void callBuiltinUnwindException(V4IR::Temp *) = 0;
    virtual void callBuiltinPushCatchScope(const QString &exceptionName) = 0;
    virtual void callBuiltinForeachIteratorObject(V4IR::Temp *arg, V4IR::Temp *result) = 0;
    virtual void callBuiltinForeachNextPropertyname(V4IR::Temp *arg, V4IR::Temp *result) = 0;
    virtual void callBuiltinPushWithScope(V4IR::Temp *arg) = 0;
    virtual void callBuiltinPopScope() = 0;
    virtual void callBuiltinDeclareVar(bool deletable, const QString &name) = 0;
    virtual void callBuiltinDefineGetterSetter(V4IR::Temp *object, const QString &name, V4IR::Temp *getter, V4IR::Temp *setter) = 0;
    virtual void callBuiltinDefineProperty(V4IR::Temp *object, const QString &name, V4IR::Expr *value) = 0;
    virtual void callBuiltinDefineArray(V4IR::Temp *result, V4IR::ExprList *args) = 0;
    virtual void callBuiltinDefineObjectLiteral(V4IR::Temp *result, V4IR::ExprList *args) = 0;
    virtual void callBuiltinSetupArgumentObject(V4IR::Temp *result) = 0;
    virtual void callBuiltinConvertThisToObject() = 0;
    virtual void callValue(V4IR::Temp *value, V4IR::ExprList *args, V4IR::Temp *result) = 0;
    virtual void callProperty(V4IR::Expr *base, const QString &name, V4IR::ExprList *args, V4IR::Temp *result) = 0;
    virtual void callSubscript(V4IR::Expr *base, V4IR::Expr *index, V4IR::ExprList *args, V4IR::Temp *result) = 0;
    virtual void convertType(V4IR::Temp *source, V4IR::Temp *target) = 0;
    virtual void constructActivationProperty(V4IR::Name *func, V4IR::ExprList *args, V4IR::Temp *result) = 0;
    virtual void constructProperty(V4IR::Temp *base, const QString &name, V4IR::ExprList *args, V4IR::Temp *result) = 0;
    virtual void constructValue(V4IR::Temp *value, V4IR::ExprList *args, V4IR::Temp *result) = 0;
    virtual void loadThisObject(V4IR::Temp *temp) = 0;
    virtual void loadQmlIdArray(V4IR::Temp *temp) = 0;
    virtual void loadQmlImportedScripts(V4IR::Temp *temp) = 0;
    virtual void loadQmlContextObject(V4IR::Temp *temp) = 0;
    virtual void loadQmlScopeObject(V4IR::Temp *temp) = 0;
    virtual void loadQmlSingleton(const QString &name, V4IR::Temp *temp) = 0;
    virtual void loadConst(V4IR::Const *sourceConst, V4IR::Temp *targetTemp) = 0;
    virtual void loadString(const QString &str, V4IR::Temp *targetTemp) = 0;
    virtual void loadRegexp(V4IR::RegExp *sourceRegexp, V4IR::Temp *targetTemp) = 0;
    virtual void getActivationProperty(const V4IR::Name *name, V4IR::Temp *temp) = 0;
    virtual void setActivationProperty(V4IR::Expr *source, const QString &targetName) = 0;
    virtual void initClosure(V4IR::Closure *closure, V4IR::Temp *target) = 0;
    virtual void getProperty(V4IR::Expr *base, const QString &name, V4IR::Temp *target) = 0;
    virtual void getQObjectProperty(V4IR::Expr *base, int propertyIndex, bool captureRequired, int attachedPropertiesId, V4IR::Temp *targetTemp) = 0;
    virtual void setProperty(V4IR::Expr *source, V4IR::Expr *targetBase, const QString &targetName) = 0;
    virtual void setQObjectProperty(V4IR::Expr *source, V4IR::Expr *targetBase, int propertyIndex) = 0;
    virtual void getElement(V4IR::Expr *base, V4IR::Expr *index, V4IR::Temp *target) = 0;
    virtual void setElement(V4IR::Expr *source, V4IR::Expr *targetBase, V4IR::Expr *targetIndex) = 0;
    virtual void copyValue(V4IR::Temp *sourceTemp, V4IR::Temp *targetTemp) = 0;
    virtual void swapValues(V4IR::Temp *sourceTemp, V4IR::Temp *targetTemp) = 0;
    virtual void unop(V4IR::AluOp oper, V4IR::Temp *sourceTemp, V4IR::Temp *targetTemp) = 0;
    virtual void binop(V4IR::AluOp oper, V4IR::Expr *leftSource, V4IR::Expr *rightSource, V4IR::Temp *target) = 0;

protected:
    virtual void callBuiltin(V4IR::Call *c, V4IR::Temp *result);

    V4IR::Function *_function; // subclass needs to set
};
} // namespace IR

} // namespace QQmlJS

QT_END_NAMESPACE

#endif // QV4ISEL_P_H
