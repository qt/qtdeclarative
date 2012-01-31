/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the QtDeclarative module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QDECLARATIVECOMPILER_P_H
#define QDECLARATIVECOMPILER_P_H

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

#include "qdeclarative.h"
#include "qdeclarativeerror.h"
#include <private/qv8_p.h>
#include "qdeclarativeinstruction_p.h"
#include "qdeclarativescript_p.h"
#include "qdeclarativeengine_p.h"
#include <private/qbitfield_p.h>
#include "qdeclarativepropertycache_p.h"
#include "qdeclarativeintegercache_p.h"
#include "qdeclarativetypenamecache_p.h"
#include "qdeclarativetypeloader_p.h"

#include <QtCore/qbytearray.h>
#include <QtCore/qset.h>
#include <QtCore/QCoreApplication>

QT_BEGIN_NAMESPACE

class QDeclarativeEngine;
class QDeclarativeComponent;
class QDeclarativeContext;
class QDeclarativeContextData;

class Q_AUTOTEST_EXPORT QDeclarativeCompiledData : public QDeclarativeRefCount,
                                                   public QDeclarativeCleanup
{
public:
    QDeclarativeCompiledData(QDeclarativeEngine *engine);
    virtual ~QDeclarativeCompiledData();

    QDeclarativeEngine *engine;

    QString name;
    QUrl url;
    QDeclarativeTypeNameCache *importCache;

    struct TypeReference 
    {
        TypeReference()
        : type(0), typePropertyCache(0), component(0) {}

        QString className;
        QDeclarativeType *type;
        QDeclarativePropertyCache *typePropertyCache;
        QDeclarativeCompiledData *component;

        const QMetaObject *metaObject() const;
        QDeclarativePropertyCache *propertyCache() const;
        QDeclarativePropertyCache *createPropertyCache(QDeclarativeEngine *);
    };
    QList<TypeReference> types;

    QList<v8::Persistent<v8::Array> > v8bindings;

    const QMetaObject *root;
    QAbstractDynamicMetaObject rootData;
    QDeclarativePropertyCache *rootPropertyCache;
    QList<QString> primitives;
    QList<QByteArray> datas;
    QByteArray bytecode;
    QList<QDeclarativePropertyCache *> propertyCaches;
    QList<QDeclarativeIntegerCache *> contextCaches;
    QList<QDeclarativeScriptData *> scripts;
    QList<QUrl> urls;

    struct Instruction {
#define QML_INSTR_DATA_TYPEDEF(I, FMT) typedef QDeclarativeInstructionData<QDeclarativeInstruction::I> I;
    FOR_EACH_QML_INSTR(QML_INSTR_DATA_TYPEDEF)
#undef QML_INSTR_DATA_TYPEDEF
    private:
        Instruction();
    };

    void dumpInstructions();

    template <int Instr>
    int addInstruction(const QDeclarativeInstructionData<Instr> &data)
    {
        QDeclarativeInstruction genericInstr;
        QDeclarativeInstructionMeta<Instr>::setData(genericInstr, data);
        return addInstructionHelper(static_cast<QDeclarativeInstruction::Type>(Instr), genericInstr);
    }
    int nextInstructionIndex();
    QDeclarativeInstruction *instruction(int index);
    QDeclarativeInstruction::Type instructionType(const QDeclarativeInstruction *instr);

    bool isInitialized() const { return hasEngine(); }
    void initialize(QDeclarativeEngine *);

protected:
    virtual void destroy(); // From QDeclarativeRefCount
    virtual void clear(); // From QDeclarativeCleanup

private:
    friend class QDeclarativeCompiler;

    int addInstructionHelper(QDeclarativeInstruction::Type type, QDeclarativeInstruction &instr);
    void dump(QDeclarativeInstruction *, int idx = -1);
    QDeclarativeCompiledData(const QDeclarativeCompiledData &other);
    QDeclarativeCompiledData &operator=(const QDeclarativeCompiledData &other);
    QByteArray packData;
    int pack(const char *, size_t);

    int indexForString(const QString &);
    int indexForByteArray(const QByteArray &);
    int indexForUrl(const QUrl &);
};

namespace QDeclarativeCompilerTypes {
    struct BindingContext 
    {
        BindingContext()
            : stack(0), owner(0), object(0) {}
        BindingContext(QDeclarativeScript::Object *o)
            : stack(0), owner(0), object(o) {}
        BindingContext incr() const {
            BindingContext rv(object);
            rv.stack = stack + 1;
            return rv;
        }
        bool isSubContext() const { return stack != 0; }
        int stack;
        int owner;
        QDeclarativeScript::Object *object;
    };

    struct BindingReference
    {
        enum DataType { QtScript, V4, V8,
                        Tr, TrId };
        DataType dataType;
    };

    struct JSBindingReference : public QDeclarativePool::Class,
                                public BindingReference
    {
        JSBindingReference() : nextReference(0) {}

        QDeclarativeScript::Variant expression;
        QDeclarativeScript::Property *property;
        QDeclarativeScript::Value *value;

        int compiledIndex;

        QString rewrittenExpression;
        BindingContext bindingContext;

        JSBindingReference *nextReference;
    };

    struct TrBindingReference : public QDeclarativePool::POD,
                                public BindingReference
    {
        QStringRef text;
        QStringRef comment;
        int n;
    };

    struct IdList : public QFieldList<QDeclarativeScript::Object, 
                                      &QDeclarativeScript::Object::nextIdObject>
    {
        QDeclarativeScript::Object *value(const QString &id) const {
            for (QDeclarativeScript::Object *o = first(); o; o = next(o)) {
                if (o->id == id)
                    return o;
            }
            return 0;
        }
    };

    struct DepthStack {
        DepthStack() : _depth(0), _maxDepth(0) {}
        DepthStack(const DepthStack &o) : _depth(o._depth), _maxDepth(o._maxDepth) {}
        DepthStack &operator=(const DepthStack &o) { _depth = o._depth; _maxDepth = o._maxDepth; return *this; }

        int depth() const { return _depth; }
        int maxDepth() const { return _maxDepth; }

        void push() { ++_depth; _maxDepth = qMax(_depth, _maxDepth); }
        void pop() { --_depth; Q_ASSERT(_depth >= 0); Q_ASSERT(_maxDepth > _depth); }

        void pushPop(int count) { _maxDepth = qMax(_depth + count, _maxDepth); }
    private:
        int _depth;
        int _maxDepth;
    };

    // Contains all the incremental compiler state about a component.  As
    // a single QML file can have multiple components defined, there may be
    // more than one of these for each compile
    struct ComponentCompileState : public QDeclarativePool::Class
    {
        ComponentCompileState() 
        : parserStatusCount(0), totalBindingsCount(0), pushedProperties(0), nested(false), 
          v8BindingProgramLine(-1), root(0) {}

        IdList ids;
        int parserStatusCount;
        int totalBindingsCount;
        int pushedProperties;
        bool nested;

        QByteArray compiledBindingData;
        QString v8BindingProgram;
        int v8BindingProgramLine;
        int v8BindingProgramIndex;

        DepthStack objectDepth;
        DepthStack listDepth;

        typedef QDeclarativeCompilerTypes::JSBindingReference B;
        typedef QFieldList<B, &B::nextReference> JSBindingReferenceList;
        JSBindingReferenceList bindings;
        typedef QDeclarativeScript::Object O;
        typedef QFieldList<O, &O::nextAliasingObject> AliasingObjectsList;
        AliasingObjectsList aliasingObjects;
        QDeclarativeScript::Object *root;
    };
};

class QMetaObjectBuilder;
class Q_AUTOTEST_EXPORT QDeclarativeCompiler
{
    Q_DECLARE_TR_FUNCTIONS(QDeclarativeCompiler)
public:
    QDeclarativeCompiler(QDeclarativePool *);

    bool compile(QDeclarativeEngine *, QDeclarativeTypeData *, QDeclarativeCompiledData *);

    bool isError() const;
    QList<QDeclarativeError> errors() const;

    static bool isAttachedPropertyName(const QString &);
    static bool isSignalPropertyName(const QString &);
    static bool isAttachedPropertyName(const QHashedStringRef &);
    static bool isSignalPropertyName(const QHashedStringRef &);

    int evaluateEnum(const QByteArray& script) const; // for QDeclarativeCustomParser::evaluateEnum
    const QMetaObject *resolveType(const QString& name) const; // for QDeclarativeCustomParser::resolveType
    int rewriteBinding(const QDeclarativeScript::Variant& value, const QString& name); // for QDeclarativeCustomParser::rewriteBinding
    QString rewriteSignalHandler(const QDeclarativeScript::Variant& value, const QString &name);  // for QDeclarativeCustomParser::rewriteSignalHandler

private:
    typedef QDeclarativeCompiledData::Instruction Instruction;

    static void reset(QDeclarativeCompiledData *);

    void compileTree(QDeclarativeScript::Object *tree);


    bool buildObject(QDeclarativeScript::Object *obj, const QDeclarativeCompilerTypes::BindingContext &);
    bool buildComponent(QDeclarativeScript::Object *obj, const QDeclarativeCompilerTypes::BindingContext &);
    bool buildSubObject(QDeclarativeScript::Object *obj, const QDeclarativeCompilerTypes::BindingContext &);
    bool buildSignal(QDeclarativeScript::Property *prop, QDeclarativeScript::Object *obj, 
                     const QDeclarativeCompilerTypes::BindingContext &);
    bool buildProperty(QDeclarativeScript::Property *prop, QDeclarativeScript::Object *obj, 
                       const QDeclarativeCompilerTypes::BindingContext &);
    bool buildPropertyInNamespace(QDeclarativeImportedNamespace *ns,
                                  QDeclarativeScript::Property *prop, 
                                  QDeclarativeScript::Object *obj, 
                                  const QDeclarativeCompilerTypes::BindingContext &);
    bool buildIdProperty(QDeclarativeScript::Property *prop, QDeclarativeScript::Object *obj);
    bool buildAttachedProperty(QDeclarativeScript::Property *prop, 
                               QDeclarativeScript::Object *obj,
                               const QDeclarativeCompilerTypes::BindingContext &ctxt);
    bool buildGroupedProperty(QDeclarativeScript::Property *prop,
                              QDeclarativeScript::Object *obj,
                              const QDeclarativeCompilerTypes::BindingContext &ctxt);
    bool buildValueTypeProperty(QObject *type, 
                                QDeclarativeScript::Object *obj, 
                                QDeclarativeScript::Object *baseObj,
                                const QDeclarativeCompilerTypes::BindingContext &ctxt);
    bool buildListProperty(QDeclarativeScript::Property *prop,
                           QDeclarativeScript::Object *obj,
                           const QDeclarativeCompilerTypes::BindingContext &ctxt);
    bool buildScriptStringProperty(QDeclarativeScript::Property *prop,
                                   QDeclarativeScript::Object *obj,
                                   const QDeclarativeCompilerTypes::BindingContext &ctxt);
    bool buildPropertyAssignment(QDeclarativeScript::Property *prop,
                                 QDeclarativeScript::Object *obj,
                                 const QDeclarativeCompilerTypes::BindingContext &ctxt);
    bool buildPropertyObjectAssignment(QDeclarativeScript::Property *prop,
                                       QDeclarativeScript::Object *obj,
                                       QDeclarativeScript::Value *value,
                                       const QDeclarativeCompilerTypes::BindingContext &ctxt);
    bool buildPropertyOnAssignment(QDeclarativeScript::Property *prop,
                                   QDeclarativeScript::Object *obj,
                                   QDeclarativeScript::Object *baseObj,
                                   QDeclarativeScript::Value *value,
                                   const QDeclarativeCompilerTypes::BindingContext &ctxt);
    bool buildPropertyLiteralAssignment(QDeclarativeScript::Property *prop,
                                        QDeclarativeScript::Object *obj,
                                        QDeclarativeScript::Value *value,
                                        const QDeclarativeCompilerTypes::BindingContext &ctxt);
    bool doesPropertyExist(QDeclarativeScript::Property *prop, QDeclarativeScript::Object *obj);
    bool testLiteralAssignment(QDeclarativeScript::Property *prop,
                               QDeclarativeScript::Value *value);
    bool testQualifiedEnumAssignment(QDeclarativeScript::Property *prop,
                                     QDeclarativeScript::Object *obj,
                                     QDeclarativeScript::Value *value,
                                     bool *isAssignment);
    enum DynamicMetaMode { IgnoreAliases, ResolveAliases, ForceCreation };
    bool mergeDynamicMetaProperties(QDeclarativeScript::Object *obj);
    bool buildDynamicMeta(QDeclarativeScript::Object *obj, DynamicMetaMode mode);
    bool checkDynamicMeta(QDeclarativeScript::Object *obj);
    bool buildBinding(QDeclarativeScript::Value *, QDeclarativeScript::Property *prop,
                      const QDeclarativeCompilerTypes::BindingContext &ctxt);
    bool buildLiteralBinding(QDeclarativeScript::Value *, QDeclarativeScript::Property *prop,
                             const QDeclarativeCompilerTypes::BindingContext &ctxt);
    bool buildComponentFromRoot(QDeclarativeScript::Object *obj, const QDeclarativeCompilerTypes::BindingContext &);
    bool compileAlias(QFastMetaBuilder &, 
                      QByteArray &data,
                      QDeclarativeScript::Object *obj, 
                      int propIndex, int aliasIndex,
                      QDeclarativeScript::Object::DynamicProperty &);
    bool completeComponentBuild();
    bool checkValidId(QDeclarativeScript::Value *, const QString &);


    void genObject(QDeclarativeScript::Object *obj);
    void genObjectBody(QDeclarativeScript::Object *obj);
    void genValueTypeProperty(QDeclarativeScript::Object *obj,QDeclarativeScript::Property *);
    void genComponent(QDeclarativeScript::Object *obj);
    void genValueProperty(QDeclarativeScript::Property *prop, QDeclarativeScript::Object *obj);
    void genListProperty(QDeclarativeScript::Property *prop, QDeclarativeScript::Object *obj);
    void genPropertyAssignment(QDeclarativeScript::Property *prop, 
                               QDeclarativeScript::Object *obj,
                               QDeclarativeScript::Property *valueTypeProperty = 0);
    void genLiteralAssignment(QDeclarativeScript::Property *prop,
                              QDeclarativeScript::Value *value);
    void genBindingAssignment(QDeclarativeScript::Value *binding, 
                              QDeclarativeScript::Property *prop, 
                              QDeclarativeScript::Object *obj,
                              QDeclarativeScript::Property *valueTypeProperty = 0);
    int genContextCache();

    QDeclarativePropertyData genValueTypeData(QDeclarativeScript::Property *prop,
                                              QDeclarativeScript::Property *valueTypeProp);

    int componentTypeRef();
    int translationContextIndex();

    static QDeclarativeType *toQmlType(QDeclarativeScript::Object *from);
    bool canCoerce(int to, QDeclarativeScript::Object *from);

    QString elementName(QDeclarativeScript::Object *);

    QStringList deferredProperties(QDeclarativeScript::Object *);

    QDeclarativePropertyData *property(QDeclarativeScript::Object *, int);
    QDeclarativePropertyData *property(QDeclarativeScript::Object *, const QHashedStringRef &,
                                       bool *notInRevision = 0);
    QDeclarativePropertyData *signal(QDeclarativeScript::Object *, const QHashedStringRef &,
                                     bool *notInRevision = 0);
    int indexOfProperty(QDeclarativeScript::Object *, const QHashedStringRef &, bool *notInRevision = 0);
    int indexOfProperty(QDeclarativeScript::Object *, const QString &, bool *notInRevision = 0);
    int indexOfSignal(QDeclarativeScript::Object *, const QString &, bool *notInRevision = 0);

    void addId(const QString &, QDeclarativeScript::Object *);

    void dumpStats();

    void addBindingReference(QDeclarativeCompilerTypes::JSBindingReference *);

    QDeclarativeCompilerTypes::ComponentCompileState *compileState;

    QDeclarativePool *pool;

    QDeclarativeCompilerTypes::ComponentCompileState *componentState(QDeclarativeScript::Object *);
    void saveComponentState();

    QList<QDeclarativeError> exceptions;
    QDeclarativeCompiledData *output;
    QDeclarativeEngine *engine;
    QDeclarativeEnginePrivate *enginePrivate;
    QDeclarativeScript::Object *unitRoot;
    QDeclarativeTypeData *unit;
    int cachedComponentTypeRef;
    int cachedTranslationContextIndex;

    // Compiler component statistics.  Only collected if QML_COMPILER_STATS=1
    struct ComponentStat
    {
        ComponentStat() : ids(0), objects(0) {}

        int lineNumber;

        int ids;
        QList<QDeclarativeScript::LocationSpan> scriptBindings;
        QList<QDeclarativeScript::LocationSpan> optimizedBindings;
        int objects;
    };
    struct ComponentStats : public QDeclarativePool::Class
    {
        ComponentStat componentStat;
        QList<ComponentStat> savedComponentStats;
    };
    ComponentStats *componentStats;
};

QT_END_NAMESPACE

#endif // QDECLARATIVECOMPILER_P_H
