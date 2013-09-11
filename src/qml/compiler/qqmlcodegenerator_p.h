/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the tools applications of the Qt Toolkit.
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
#ifndef QQMLCODEGENERATOR_P_H
#define QQMLCODEGENERATOR_P_H

#include <private/qqmljsast_p.h>
#include <private/qqmlpool_p.h>
#include <private/qqmlscript_p.h>
#include <private/qqmljsengine_p.h>
#include <private/qv4compiler_p.h>
#include <private/qv4compileddata_p.h>
#include <private/qqmljsmemorypool_p.h>
#include <private/qv4codegen_p.h>
#include <private/qv4compiler_p.h>
#include <QTextStream>
#include <QCoreApplication>

QT_BEGIN_NAMESPACE

namespace QtQml {

using namespace QQmlJS;

struct DebugStream
{
    DebugStream(QTextStream &stream)
        : stream(stream)
        , indent(0)
    {}

    template <typename T>
    QTextStream &operator<<(const T &value)
    {
        return stream << QByteArray(indent * 4, ' ') << value;
    }

    QTextStream &noindent() { return stream; }

    QTextStream &stream;
    int indent;
};

template <typename T>
struct PoolList
{
    PoolList()
        : first(0)
        , last(0)
        , count(0)
    {}

    T *first;
    T *last;
    int count;

    void append(T *item) {
        item->next = 0;
        if (last)
            last->next = item;
        else
            first = item;
        last = item;
        ++count;
    }
};

struct QmlObject;

struct SignalParameter : public QV4::CompiledData::Parameter
{
    SignalParameter *next;
};

struct Signal
{
    int nameIndex;
    QV4::CompiledData::Location location;
    PoolList<SignalParameter> *parameters;
    Signal *next;
};

struct QmlProperty : public QV4::CompiledData::Property
{
    QmlProperty *next;
};

struct Binding : public QV4::CompiledData::Binding
{
    Binding *next;
};

struct Function
{
    int index;
    Function *next;
};

struct QmlObject
{
    int inheritedTypeNameIndex;
    int idIndex;
    int indexOfDefaultProperty;

    QV4::CompiledData::Location location;

    PoolList<QmlProperty> *properties;
    PoolList<Signal> *qmlSignals;
    PoolList<Binding> *bindings;
    PoolList<Function> *functions;

    void dump(DebugStream &out);
};

struct ParsedQML
{
    ParsedQML()
        : jsGenerator(&jsModule, sizeof(QV4::CompiledData::QmlUnit))
    {}
    QString code;
    QQmlJS::Engine jsParserEngine;
    V4IR::Module jsModule;
    QList<QV4::CompiledData::Import*> imports;
    int indexOfRootObject;
    QList<QmlObject*> objects;
    QList<AST::Node*> functions; // FunctionDeclaration, Statement or Expression
    QV4::Compiler::JSUnitGenerator jsGenerator;

    QString stringAt(int index) const { return jsGenerator.strings.value(index); }
};

// Doesn't really generate code per-se, but more the data structure
struct Q_QML_EXPORT QQmlCodeGenerator : public AST::Visitor
{
    Q_DECLARE_TR_FUNCTIONS(QQmlCodeGenerator)
public:
    QQmlCodeGenerator();
    bool generateFromQml(const QString &code, const QUrl &url, const QString &urlString, ParsedQML *output);

    using AST::Visitor::visit;
    using AST::Visitor::endVisit;

    virtual bool visit(AST::UiArrayMemberList *ast);
    virtual bool visit(AST::UiImport *ast);
    virtual bool visit(AST::UiImportList *ast);
    virtual bool visit(AST::UiObjectInitializer *ast);
    virtual bool visit(AST::UiObjectMemberList *ast);
    virtual bool visit(AST::UiParameterList *ast);
    virtual bool visit(AST::UiProgram *);
    virtual bool visit(AST::UiQualifiedId *ast);
    virtual bool visit(AST::UiArrayBinding *ast);
    virtual bool visit(AST::UiObjectBinding *ast);
    virtual bool visit(AST::UiObjectDefinition *ast);
    virtual bool visit(AST::UiPublicMember *ast);
    virtual bool visit(AST::UiScriptBinding *ast);
    virtual bool visit(AST::UiSourceElement *ast);

    void accept(AST::Node *node);

    // returns index in _objects
    int defineQMLObject(AST::UiQualifiedId *qualifiedTypeNameId, AST::UiObjectInitializer *initializer);
    int defineQMLObject(AST::UiObjectDefinition *node)
    { return defineQMLObject(node->qualifiedTypeNameId, node->initializer); }

    static QString asString(AST::UiQualifiedId *node);
    QStringRef asStringRef(AST::Node *node);
    static void extractVersion(QStringRef string, int *maj, int *min);
    QStringRef textRefAt(const AST::SourceLocation &loc) const
    { return QStringRef(&sourceCode, loc.offset, loc.length); }
    QStringRef textRefAt(const AST::SourceLocation &first,
                         const AST::SourceLocation &last) const;
    static QQmlScript::LocationSpan location(AST::UiQualifiedId *id)
    {
        return location(id->identifierToken, id->identifierToken);
    }

    void setBindingValue(QV4::CompiledData::Binding *binding, AST::Statement *statement);

    void appendBinding(const AST::SourceLocation &nameLocation, int propertyNameIndex, AST::Statement *value);
    void appendBinding(const AST::SourceLocation &nameLocation, int propertyNameIndex, int objectIndex);

    bool sanityCheckPropertyName(const AST::SourceLocation &nameLocation, int nameIndex);

    void recordError(const AST::SourceLocation &location, const QString &description);

    static QQmlScript::LocationSpan location(AST::SourceLocation start, AST::SourceLocation end);

    int registerString(const QString &str) const { return jsGenerator->registerString(str); }
    template <typename _Tp> _Tp *New() { return new (pool->allocate(sizeof(_Tp))) _Tp(); }

    QList<QQmlError> errors;

    QList<QV4::CompiledData::Import*> _imports;
    QList<QmlObject*> _objects;
    QList<AST::Node*> _functions;

    QmlObject *_object;
    QSet<QString> _propertyNames;
    QSet<QString> _signalNames;

    QQmlJS::MemoryPool *pool;
    QString sourceCode;
    QUrl url;
    QV4::Compiler::JSUnitGenerator *jsGenerator;
    bool sanityCheckFunctionNames();
};

struct Q_QML_EXPORT QmlUnitGenerator
{
    QmlUnitGenerator()
        : jsUnitGenerator(0)
    {
    }

    QV4::CompiledData::QmlUnit *generate(ParsedQML &output);

private:
    int getStringId(const QString &str) const;

    QV4::Compiler::JSUnitGenerator *jsUnitGenerator;
};

struct Q_QML_EXPORT JSCodeGen : public QQmlJS::Codegen
{
    JSCodeGen()
        : QQmlJS::Codegen(/*strict mode*/false)
    {}

    void generateJSCodeForFunctionsAndBindings(const QString &fileName, ParsedQML *output);

private:
    V4IR::Module jsModule;
};

} // namespace QtQml

QT_END_NAMESPACE

#endif // QQMLCODEGENERATOR_P_H
