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

using namespace QV4;

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

    int append(T *item) {
        item->next = 0;
        if (last)
            last->next = item;
        else
            first = item;
        last = item;
        return count++;
    }

    void prepend(T *item) {
        item->next = first;
        first = item;
        if (!last)
            last = first;
        ++count;
    }

    template <typename Sortable, typename Base, Sortable Base::*sortMember>
    T *findSortedInsertionPoint(T *item) const
    {
        T *insertPos = 0;

        for (T *it = first; it; it = it->next) {
            if (!(it->*sortMember < item->*sortMember))
                break;
            insertPos = it;
        }

        return insertPos;
    }

    void insertAfter(T *insertionPoint, T *item) {
        if (!insertionPoint) {
            prepend(item);
        } else if (insertionPoint == last) {
            append(item);
        } else {
            item->next = insertionPoint->next;
            insertionPoint->next = item;
            ++count;
        }
    }

    T *unlink(T *before, T *item) {
        T * const newNext = item->next;

        if (before)
            before->next = newNext;
        else
            first = newNext;

        if (item == last) {
            if (newNext)
                last = newNext;
            else
                last = first;
        }

        --count;
        return newNext;
    }

    T *slowAt(int index) const
    {
        T *result = first;
        while (index > 0 && result) {
            result = result->next;
            --index;
        }
        return result;
    }
};

template <typename T>
class FixedPoolArray
{
    T *data;
public:
    int count;

    void init(QQmlJS::MemoryPool *pool, const QVector<T> &vector)
    {
        count = vector.count();
        data = reinterpret_cast<T*>(pool->allocate(count * sizeof(T)));

        if (QTypeInfo<T>::isComplex) {
            for (int i = 0; i < count; ++i)
                new (data + i) T(vector.at(i));
        } else {
            memcpy(data, static_cast<const void*>(vector.constData()), count * sizeof(T));
        }
    }

    const T &at(int index) const {
        Q_ASSERT(index >= 0 && index < count);
        return data[index];
    }

    T &operator[](int index) {
        Q_ASSERT(index >= 0 && index < count);
        return data[index];
    }


    int indexOf(const T &value) const {
        for (int i = 0; i < count; ++i)
            if (data[i] == value)
                return i;
        return -1;
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
    // Binding's compiledScriptIndex is index in object's functionsAndExpressions
    Binding *next;
};

struct Function
{
    QQmlJS::AST::FunctionDeclaration *functionDeclaration;
    QV4::CompiledData::Location location;
    int nameIndex;
    quint32 index; // index in parsedQML::functions
    Function *next;
};

struct CompiledFunctionOrExpression
{
    CompiledFunctionOrExpression()
        : node(0)
        , nameIndex(0)
        , disableAcceleratedLookups(false)
        , next(0)
    {}
    CompiledFunctionOrExpression(QQmlJS::AST::Node *n)
        : node(n)
        , nameIndex(0)
        , disableAcceleratedLookups(false)
        , next(0)
    {}
    QQmlJS::AST::Node *node; // FunctionDeclaration, Statement or Expression
    quint32 nameIndex;
    bool disableAcceleratedLookups;
    CompiledFunctionOrExpression *next;
};

struct QmlObject
{
    Q_DECLARE_TR_FUNCTIONS(QmlObject)
public:
    quint32 inheritedTypeNameIndex;
    quint32 idIndex;
    int indexOfDefaultProperty;

    QV4::CompiledData::Location location;
    QV4::CompiledData::Location locationOfIdProperty;

    const QmlProperty *firstProperty() const { return properties->first; }
    int propertyCount() const { return properties->count; }
    const Signal *firstSignal() const { return qmlSignals->first; }
    int signalCount() const { return qmlSignals->count; }
    Binding *firstBinding() const { return bindings->first; }
    int bindingCount() const { return bindings->count; }
    const Function *firstFunction() const { return functions->first; }
    int functionCount() const { return functions->count; }

    // If set, then declarations for this object (and init bindings for these) should go into the
    // specified object. Used for declarations inside group properties.
    QmlObject *declarationsOverride;

    void init(QQmlJS::MemoryPool *pool, int typeNameIndex, int id, const QQmlJS::AST::SourceLocation &location = QQmlJS::AST::SourceLocation());

    void dump(DebugStream &out);

    QString sanityCheckFunctionNames(const QSet<QString> &illegalNames, QQmlJS::AST::SourceLocation *errorLocation);

    QString appendSignal(Signal *signal);
    QString appendProperty(QmlProperty *prop, const QString &propertyName, bool isDefaultProperty, const QQmlJS::AST::SourceLocation &defaultToken, QQmlJS::AST::SourceLocation *errorLocation);
    void appendFunction(QtQml::Function *f);

    QString appendBinding(Binding *b, bool isListBinding);
    Binding *findBinding(quint32 nameIndex) const;
    Binding *unlinkBinding(Binding *before, Binding *binding) { return bindings->unlink(before, binding); }
    void insertSorted(Binding *b);

    PoolList<CompiledFunctionOrExpression> *functionsAndExpressions;
    FixedPoolArray<int> *runtimeFunctionIndices;

private:
    PoolList<QmlProperty> *properties;
    PoolList<Signal> *qmlSignals;
    PoolList<Binding> *bindings;
    PoolList<Function> *functions;
};

struct Pragma
{
    enum PragmaType {
        PragmaSingleton = 0x1
    };
    quint32 type;

    QV4::CompiledData::Location location;
};

struct ParsedQML
{
    ParsedQML(bool debugMode)
        : jsModule(debugMode)
        , jsGenerator(&jsModule, sizeof(QV4::CompiledData::QmlUnit))
    {}
    QString code;
    QQmlJS::Engine jsParserEngine;
    QV4::IR::Module jsModule;
    QList<QV4::CompiledData::Import*> imports;
    QList<Pragma*> pragmas;
    QQmlJS::AST::UiProgram *program;
    int indexOfRootObject;
    QList<QmlObject*> objects;
    QV4::Compiler::JSUnitGenerator jsGenerator;

    QV4::CompiledData::TypeReferenceMap typeReferences;

    QString stringAt(int index) const { return jsGenerator.strings.value(index); }
};

// Doesn't really generate code per-se, but more the data structure
struct Q_QML_EXPORT QQmlCodeGenerator : public QQmlJS::AST::Visitor
{
    Q_DECLARE_TR_FUNCTIONS(QQmlCodeGenerator)
public:
    QQmlCodeGenerator(const QSet<QString> &illegalNames);
    bool generateFromQml(const QString &code, const QUrl &url, const QString &urlString, ParsedQML *output);

    static bool isSignalPropertyName(const QString &name);

    using QQmlJS::AST::Visitor::visit;
    using QQmlJS::AST::Visitor::endVisit;

    virtual bool visit(QQmlJS::AST::UiArrayMemberList *ast);
    virtual bool visit(QQmlJS::AST::UiImport *ast);
    virtual bool visit(QQmlJS::AST::UiPragma *ast);
    virtual bool visit(QQmlJS::AST::UiHeaderItemList *ast);
    virtual bool visit(QQmlJS::AST::UiObjectInitializer *ast);
    virtual bool visit(QQmlJS::AST::UiObjectMemberList *ast);
    virtual bool visit(QQmlJS::AST::UiParameterList *ast);
    virtual bool visit(QQmlJS::AST::UiProgram *);
    virtual bool visit(QQmlJS::AST::UiQualifiedId *ast);
    virtual bool visit(QQmlJS::AST::UiArrayBinding *ast);
    virtual bool visit(QQmlJS::AST::UiObjectBinding *ast);
    virtual bool visit(QQmlJS::AST::UiObjectDefinition *ast);
    virtual bool visit(QQmlJS::AST::UiPublicMember *ast);
    virtual bool visit(QQmlJS::AST::UiScriptBinding *ast);
    virtual bool visit(QQmlJS::AST::UiSourceElement *ast);

    void accept(QQmlJS::AST::Node *node);

    // returns index in _objects
    bool defineQMLObject(int *objectIndex, QQmlJS::AST::UiQualifiedId *qualifiedTypeNameId, const QQmlJS::AST::SourceLocation &location, QQmlJS::AST::UiObjectInitializer *initializer, QmlObject *declarationsOverride = 0);
    bool defineQMLObject(int *objectIndex, QQmlJS::AST::UiObjectDefinition *node, QmlObject *declarationsOverride = 0)
    { return defineQMLObject(objectIndex, node->qualifiedTypeNameId, node->qualifiedTypeNameId->firstSourceLocation(), node->initializer, declarationsOverride); }

    static QString asString(QQmlJS::AST::UiQualifiedId *node);
    QStringRef asStringRef(QQmlJS::AST::Node *node);
    static void extractVersion(QStringRef string, int *maj, int *min);
    QStringRef textRefAt(const QQmlJS::AST::SourceLocation &loc) const
    { return QStringRef(&sourceCode, loc.offset, loc.length); }
    QStringRef textRefAt(const QQmlJS::AST::SourceLocation &first,
                         const QQmlJS::AST::SourceLocation &last) const;
    static QQmlScript::LocationSpan location(QQmlJS::AST::UiQualifiedId *id)
    {
        return location(id->identifierToken, id->identifierToken);
    }

    void setBindingValue(QV4::CompiledData::Binding *binding, QQmlJS::AST::Statement *statement);

    void appendBinding(QQmlJS::AST::UiQualifiedId *name, QQmlJS::AST::Statement *value);
    void appendBinding(QQmlJS::AST::UiQualifiedId *name, int objectIndex, bool isOnAssignment = false);
    void appendBinding(const QQmlJS::AST::SourceLocation &qualifiedNameLocation, const QQmlJS::AST::SourceLocation &nameLocation, quint32 propertyNameIndex, QQmlJS::AST::Statement *value);
    void appendBinding(const QQmlJS::AST::SourceLocation &qualifiedNameLocation, const QQmlJS::AST::SourceLocation &nameLocation, quint32 propertyNameIndex, int objectIndex, bool isListItem = false, bool isOnAssignment = false);

    QmlObject *bindingsTarget() const;

    bool setId(const QQmlJS::AST::SourceLocation &idLocation, QQmlJS::AST::Statement *value);

    // resolves qualified name (font.pixelSize for example) and returns the last name along
    // with the object any right-hand-side of a binding should apply to.
    bool resolveQualifiedId(QQmlJS::AST::UiQualifiedId **nameToResolve, QmlObject **object, bool onAssignment = false);

    void recordError(const QQmlJS::AST::SourceLocation &location, const QString &description);

    void collectTypeReferences();

    static QQmlScript::LocationSpan location(QQmlJS::AST::SourceLocation start, QQmlJS::AST::SourceLocation end);

    quint32 registerString(const QString &str) const { return jsGenerator->registerString(str); }
    template <typename _Tp> _Tp *New() { return pool->New<_Tp>(); }

    QString stringAt(int index) const { return jsGenerator->strings.at(index); }

    static bool isStatementNodeScript(QQmlJS::AST::Statement *statement);

    QList<QQmlError> errors;

    QSet<QString> illegalNames;

    QList<QV4::CompiledData::Import*> _imports;
    QList<Pragma*> _pragmas;
    QList<QmlObject*> _objects;

    QV4::CompiledData::TypeReferenceMap _typeReferences;

    QmlObject *_object;
    QmlProperty *_propertyDeclaration;

    QQmlJS::MemoryPool *pool;
    QString sourceCode;
    QUrl url;
    QV4::Compiler::JSUnitGenerator *jsGenerator;
};

struct Q_QML_EXPORT QmlUnitGenerator
{
    QmlUnitGenerator()
        : jsUnitGenerator(0)
    {
    }

    QV4::CompiledData::QmlUnit *generate(ParsedQML &output);

private:
    typedef bool (Binding::*BindingFilter)() const;
    char *writeBindings(char *bindingPtr, QmlObject *o, BindingFilter filter) const;

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

    QQmlPropertyData *property(const QString &name, bool *notInRevision = 0, QObject *object = 0, QQmlContextData *context = 0);

    // This code must match the semantics of QQmlPropertyPrivate::findSignalByName
    QQmlPropertyData *signal(const QString &name, bool *notInRevision, QObject *object = 0, QQmlContextData *context = 0);

    QQmlPropertyCache *cache;
};

struct Q_QML_EXPORT JSCodeGen : public QQmlJS::Codegen
{
    JSCodeGen(const QString &fileName, const QString &sourceCode, IR::Module *jsModule,
              QQmlJS::Engine *jsEngine, QQmlJS::AST::UiProgram *qmlRoot, QQmlTypeNameCache *imports,
              const QStringList &stringPool);

    struct IdMapping
    {
        QString name;
        int idIndex;
        QQmlPropertyCache *type;
    };
    typedef QVector<IdMapping> ObjectIdMapping;

    void beginContextScope(const ObjectIdMapping &objectIds, QQmlPropertyCache *contextObject);
    void beginObjectScope(QQmlPropertyCache *scopeObject);

    // Returns mapping from input functions to index in IR::Module::functions / compiledData->runtimeFunctions
    QVector<int> generateJSCodeForFunctionsAndBindings(const QList<CompiledFunctionOrExpression> &functions);

protected:
    virtual void beginFunctionBodyHook();
    virtual IR::Expr *fallbackNameLookup(const QString &name, int line, int col);

private:
    QQmlPropertyData *lookupQmlCompliantProperty(QQmlPropertyCache *cache, const QString &name, bool *propertyExistsButForceNameLookup = 0);

    QString sourceCode;
    QQmlJS::Engine *jsEngine; // needed for memory pool
    QQmlJS::AST::UiProgram *qmlRoot;
    QQmlTypeNameCache *imports;
    const QStringList &stringPool;

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
