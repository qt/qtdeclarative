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

class QQmlTypeNameCache;

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

    QStringList parameterStringList(const QStringList &stringPool) const;

    Signal *next;
};

struct QmlProperty : public QV4::CompiledData::Property
{
    QmlProperty *next;
};

struct Binding : public QV4::CompiledData::Binding
{
    // Binding's compiledScriptIndex is index in parsedQML::functions
    Binding *next;
};

struct Function
{
    int index; // index in parsedQML::functions
    Function *next;
};

struct QmlObject
{
    int inheritedTypeNameIndex;
    int idIndex;
    int indexOfDefaultProperty;

    QV4::CompiledData::Location location;
    QV4::CompiledData::Location locationOfIdProperty;

    PoolList<QmlProperty> *properties;
    PoolList<Signal> *qmlSignals;
    PoolList<Binding> *bindings;
    PoolList<Function> *functions;

    void dump(DebugStream &out);
};

struct Pragma
{
    enum PragmaType {
        PragmaSingleton = 0x1
    };
    quint32 type;

    QV4::CompiledData::Location location;
};

struct CompiledFunctionOrExpression
{
    CompiledFunctionOrExpression()
        : disableAcceleratedLookups(false)
    {}
    CompiledFunctionOrExpression(AST::Node *n)
        : node(n)
        , disableAcceleratedLookups(false)
    {}
    AST::Node *node; // FunctionDeclaration, Statement or Expression
    QString name;
    bool disableAcceleratedLookups;
};

struct ParsedQML
{
    ParsedQML(bool debugMode)
        : jsModule(debugMode)
        , jsGenerator(&jsModule, sizeof(QV4::CompiledData::QmlUnit))
    {}
    QString code;
    QQmlJS::Engine jsParserEngine;
    V4IR::Module jsModule;
    QList<QV4::CompiledData::Import*> imports;
    QList<Pragma*> pragmas;
    AST::UiProgram *program;
    int indexOfRootObject;
    QList<QmlObject*> objects;
    QList<CompiledFunctionOrExpression> functions;
    QV4::Compiler::JSUnitGenerator jsGenerator;

    QV4::CompiledData::TypeReferenceMap typeReferences;

    QString stringAt(int index) const { return jsGenerator.strings.value(index); }
};

// Doesn't really generate code per-se, but more the data structure
struct Q_QML_EXPORT QQmlCodeGenerator : public AST::Visitor
{
    Q_DECLARE_TR_FUNCTIONS(QQmlCodeGenerator)
public:
    QQmlCodeGenerator();
    bool generateFromQml(const QString &code, const QUrl &url, const QString &urlString, ParsedQML *output);

    static bool isSignalPropertyName(const QString &name);

    using AST::Visitor::visit;
    using AST::Visitor::endVisit;

    virtual bool visit(AST::UiArrayMemberList *ast);
    virtual bool visit(AST::UiImport *ast);
    virtual bool visit(AST::UiPragma *ast);
    virtual bool visit(AST::UiHeaderItemList *ast);
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

    void appendBinding(AST::UiQualifiedId *name, AST::Statement *value);
    void appendBinding(AST::UiQualifiedId *name, int objectIndex);
    void appendBinding(const AST::SourceLocation &nameLocation, int propertyNameIndex, AST::Statement *value);
    void appendBinding(const AST::SourceLocation &nameLocation, int propertyNameIndex, int objectIndex, bool isListItem = false);

    bool setId(AST::Statement *value);

    // resolves qualified name (font.pixelSize for example) and returns the last name along
    // with the object any right-hand-side of a binding should apply to.
    bool resolveQualifiedId(AST::UiQualifiedId **nameToResolve, QmlObject **object);

    bool sanityCheckPropertyName(const AST::SourceLocation &nameLocation, int nameIndex, bool isListItem = false);

    void recordError(const AST::SourceLocation &location, const QString &description);

    void collectTypeReferences();

    static QQmlScript::LocationSpan location(AST::SourceLocation start, AST::SourceLocation end);

    int registerString(const QString &str) const { return jsGenerator->registerString(str); }
    template <typename _Tp> _Tp *New() { return new (pool->allocate(sizeof(_Tp))) _Tp(); }

    QString stringAt(int index) const { return jsGenerator->strings.at(index); }

    static bool isStatementNodeScript(AST::Statement *statement);

    QList<QQmlError> errors;

    QList<QV4::CompiledData::Import*> _imports;
    QList<Pragma*> _pragmas;
    QList<QmlObject*> _objects;
    QList<CompiledFunctionOrExpression> _functions;

    QV4::CompiledData::TypeReferenceMap _typeReferences;

    QmlObject *_object;
    QSet<QString> _propertyNames;
    QSet<QString> _signalNames;

    QQmlJS::MemoryPool *pool;
    QString sourceCode;
    QUrl url;
    QV4::Compiler::JSUnitGenerator *jsGenerator;
    int emptyStringIndex;
    bool sanityCheckFunctionNames();
};

struct Q_QML_EXPORT QmlUnitGenerator
{
    QmlUnitGenerator()
        : jsUnitGenerator(0)
    {
    }

    QV4::CompiledData::QmlUnit *generate(ParsedQML &output, const QVector<int> &runtimeFunctionIndices);

private:
    int getStringId(const QString &str) const;

    QV4::Compiler::JSUnitGenerator *jsUnitGenerator;
};

struct PropertyResolver
{
    PropertyResolver(QQmlPropertyCache *cache)
        : cache(cache)
    {}

    QQmlPropertyData *property(int index)
    {
        return cache->property(index);
    }

    QQmlPropertyData *property(const QString &name, bool *notInRevision = 0);

    // This code must match the semantics of QQmlPropertyPrivate::findSignalByName
    QQmlPropertyData *signal(const QString &name, bool *notInRevision);

    QQmlPropertyCache *cache;
};

// "Converts" signal expressions to full-fleged function declarations with
// parameters taken from the signal declarations
// It also updates the QV4::CompiledData::Binding objects to set the property name
// to the final signal name (onTextChanged -> textChanged) and sets the IsSignalExpression flag.
struct SignalHandlerConverter
{
    Q_DECLARE_TR_FUNCTIONS(QQmlCodeGenerator)
public:
    SignalHandlerConverter(QQmlEnginePrivate *enginePrivate, ParsedQML *parsedQML,
                           QQmlCompiledData *unit);

    bool convertSignalHandlerExpressionsToFunctionDeclarations();

    QList<QQmlError> errors;

private:
    bool convertSignalHandlerExpressionsToFunctionDeclarations(QmlObject *obj, const QString &typeName, QQmlPropertyCache *propertyCache);

    const QString &stringAt(int index) const { return parsedQML->jsGenerator.strings.at(index); }
    void recordError(const QV4::CompiledData::Location &location, const QString &description);

    QQmlEnginePrivate *enginePrivate;
    ParsedQML *parsedQML;
    QQmlCompiledData *unit;
};

struct Q_QML_EXPORT JSCodeGen : public QQmlJS::Codegen
{
    JSCodeGen(const QString &fileName, const QString &sourceCode, V4IR::Module *jsModule,
              QQmlJS::Engine *jsEngine, AST::UiProgram *qmlRoot, QQmlTypeNameCache *imports);

    struct IdMapping
    {
        QString name;
        int idIndex;
        QQmlPropertyCache *type;
    };
    typedef QVector<IdMapping> ObjectIdMapping;

    void beginContextScope(const ObjectIdMapping &objectIds, QQmlPropertyCache *contextObject);
    void beginObjectScope(QQmlPropertyCache *scopeObject);

    // Returns mapping from input functions to index in V4IR::Module::functions / compiledData->runtimeFunctions
    QVector<int> generateJSCodeForFunctionsAndBindings(const QList<CompiledFunctionOrExpression> &functions);

protected:
    virtual void beginFunctionBodyHook();
    virtual V4IR::Expr *fallbackNameLookup(const QString &name, int line, int col);

private:
    QQmlPropertyData *lookupQmlCompliantProperty(QQmlPropertyCache *cache, const QString &name, bool *propertyExistsButForceNameLookup = 0);

    QString sourceCode;
    QQmlJS::Engine *jsEngine; // needed for memory pool
    AST::UiProgram *qmlRoot;
    QQmlTypeNameCache *imports;

    bool _disableAcceleratedLookups;
    ObjectIdMapping _idObjects;
    QQmlPropertyCache *_contextObject;
    QQmlPropertyCache *_scopeObject;
    int _contextObjectTemp;
    int _scopeObjectTemp;
    int _importedScriptsTemp;
    int _idArrayTemp;
};

} // namespace QtQml

QT_END_NAMESPACE

#endif // QQMLCODEGENERATOR_P_H
