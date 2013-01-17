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

#ifndef QQMLCOMPILER_P_H
#define QQMLCOMPILER_P_H

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

#include "qqml.h"
#include "qqmlerror.h"
#include <private/qv8_p.h>
#include "qqmlinstruction_p.h"
#include "qqmlscript_p.h"
#include "qqmlengine_p.h"
#include <private/qbitfield_p.h>
#include "qqmlpropertycache_p.h"
#include "qqmlintegercache_p.h"
#include "qqmltypenamecache_p.h"
#include "qqmltypeloader_p.h"

#include <QtCore/qbytearray.h>
#include <QtCore/qset.h>
#include <QtCore/QCoreApplication>

QT_BEGIN_NAMESPACE

class QQmlEngine;
class QQmlComponent;
class QQmlContext;
class QQmlContextData;

class Q_AUTOTEST_EXPORT QQmlCompiledData : public QQmlRefCount, public QQmlCleanup
{
public:
    QQmlCompiledData(QQmlEngine *engine);
    virtual ~QQmlCompiledData();

    QQmlEngine *engine;

    QString name;
    QUrl url;
    QQmlTypeNameCache *importCache;

    int metaTypeId;
    int listMetaTypeId;
    bool isRegisteredWithEngine;

    struct TypeReference 
    {
        TypeReference()
        : type(0), typePropertyCache(0), component(0) {}

        QQmlType *type;
        QQmlPropertyCache *typePropertyCache;
        QQmlCompiledData *component;

        QQmlPropertyCache *propertyCache() const;
        QQmlPropertyCache *createPropertyCache(QQmlEngine *);
    };
    QList<TypeReference> types;

    struct V8Program {
        V8Program(const QByteArray &p, QQmlCompiledData *c)
        : program(p), cdata(c) {}

        QByteArray program;
        v8::Persistent<v8::Array> bindings;
        QQmlCompiledData *cdata;
    };

    QList<V8Program> programs;

    QQmlPropertyCache *rootPropertyCache;
    QList<QString> primitives;
    QList<QByteArray> datas;
    QByteArray bytecode;
    QList<QQmlPropertyCache *> propertyCaches;
    QList<QQmlIntegerCache *> contextCaches;
    QList<QQmlScriptData *> scripts;
    QList<QUrl> urls;

    struct Instruction {
#define QML_INSTR_DATA_TYPEDEF(I, FMT) typedef QQmlInstructionData<QQmlInstruction::I> I;
    FOR_EACH_QML_INSTR(QML_INSTR_DATA_TYPEDEF)
#undef QML_INSTR_DATA_TYPEDEF
    private:
        Instruction();
    };

    void dumpInstructions();

    template <int Instr>
    int addInstruction(const QQmlInstructionData<Instr> &data)
    {
        QQmlInstruction genericInstr;
        QQmlInstructionMeta<Instr>::setData(genericInstr, data);
        return addInstructionHelper(static_cast<QQmlInstruction::Type>(Instr), genericInstr);
    }
    int nextInstructionIndex();
    QQmlInstruction *instruction(int index);
    QQmlInstruction::Type instructionType(const QQmlInstruction *instr);

    bool isInitialized() const { return hasEngine(); }
    void initialize(QQmlEngine *);

protected:
    virtual void destroy(); // From QQmlRefCount
    virtual void clear(); // From QQmlCleanup

private:
    friend class QQmlCompiler;

    int addInstructionHelper(QQmlInstruction::Type type, QQmlInstruction &instr);
    void dump(QQmlInstruction *, int idx = -1);
    QQmlCompiledData(const QQmlCompiledData &other);
    QQmlCompiledData &operator=(const QQmlCompiledData &other);

    int indexForString(const QString &);
    int indexForByteArray(const QByteArray &);
    int indexForUrl(const QUrl &);
};

namespace QQmlCompilerTypes {
    struct BindingContext 
    {
        BindingContext()
            : stack(0), owner(0), object(0) {}
        BindingContext(QQmlScript::Object *o)
            : stack(0), owner(0), object(o) {}
        BindingContext incr() const {
            BindingContext rv(object);
            rv.stack = stack + 1;
            return rv;
        }
        bool isSubContext() const { return stack != 0; }
        int stack;
        int owner;
        QQmlScript::Object *object;
    };

    struct BindingReference
    {
        enum DataType { QtScript, V4, V8,
                        Tr, TrId };
        DataType dataType;
    };

    struct JSBindingReference : public QQmlPool::Class,
                                public BindingReference
    {
        JSBindingReference() : isSafe(false), nextReference(0) {}

        QQmlScript::Variant expression;
        QQmlScript::Property *property;
        QQmlScript::Value *value;

        int compiledIndex:15;
        int sharedIndex:15;
        bool isSafe:1;

        QString rewrittenExpression;
        BindingContext bindingContext;

        JSBindingReference *nextReference;
    };

    struct TrBindingReference : public QQmlPool::POD,
                                public BindingReference
    {
        QStringRef text;
        QStringRef comment;
        int n;
    };

    struct IdList : public QFieldList<QQmlScript::Object, 
                                      &QQmlScript::Object::nextIdObject>
    {
        QQmlScript::Object *value(const QString &id) const {
            for (QQmlScript::Object *o = first(); o; o = next(o)) {
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
    struct ComponentCompileState : public QQmlPool::Class
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
        QByteArray v8BindingProgram;
        int v8BindingProgramLine;

        DepthStack objectDepth;
        DepthStack listDepth;

        typedef QQmlCompilerTypes::JSBindingReference B;
        typedef QFieldList<B, &B::nextReference> JSBindingReferenceList;
        JSBindingReferenceList bindings;
        typedef QQmlScript::Object O;
        typedef QFieldList<O, &O::nextAliasingObject> AliasingObjectsList;
        AliasingObjectsList aliasingObjects;
        QQmlScript::Object *root;
    };
};

class QMetaObjectBuilder;
class Q_AUTOTEST_EXPORT QQmlCompiler
{
    Q_DECLARE_TR_FUNCTIONS(QQmlCompiler)
public:
    QQmlCompiler(QQmlPool *);

    bool compile(QQmlEngine *, QQmlTypeData *, QQmlCompiledData *);

    bool isError() const;
    QList<QQmlError> errors() const;

    static bool isAttachedPropertyName(const QString &);
    static bool isSignalPropertyName(const QString &);
    static bool isAttachedPropertyName(const QHashedStringRef &);
    static bool isSignalPropertyName(const QHashedStringRef &);

    int evaluateEnum(const QHashedStringRef &scope, const QByteArray& enumValue, bool *ok) const; // for QQmlCustomParser::evaluateEnum
    const QMetaObject *resolveType(const QString& name) const; // for QQmlCustomParser::resolveType
    int rewriteBinding(const QQmlScript::Variant& value, const QString& name); // for QQmlCustomParser::rewriteBinding
    QString rewriteSignalHandler(const QQmlScript::Variant& value, const QString &name);  // for QQmlCustomParser::rewriteSignalHandler

private:
    typedef QQmlCompiledData::Instruction Instruction;

    static void reset(QQmlCompiledData *);

    void compileTree(QQmlScript::Object *tree);


    bool buildObject(QQmlScript::Object *obj, const QQmlCompilerTypes::BindingContext &);
    bool buildComponent(QQmlScript::Object *obj, const QQmlCompilerTypes::BindingContext &);
    bool buildSubObject(QQmlScript::Object *obj, const QQmlCompilerTypes::BindingContext &);
    bool buildSignal(QQmlScript::Property *prop, QQmlScript::Object *obj, 
                     const QQmlCompilerTypes::BindingContext &);
    bool buildProperty(QQmlScript::Property *prop, QQmlScript::Object *obj, 
                       const QQmlCompilerTypes::BindingContext &);
    bool buildPropertyInNamespace(QQmlImportNamespace *ns,
                                  QQmlScript::Property *prop, 
                                  QQmlScript::Object *obj, 
                                  const QQmlCompilerTypes::BindingContext &);
    bool buildIdProperty(QQmlScript::Property *prop, QQmlScript::Object *obj);
    bool buildAttachedProperty(QQmlScript::Property *prop, 
                               QQmlScript::Object *obj,
                               const QQmlCompilerTypes::BindingContext &ctxt);
    bool buildGroupedProperty(QQmlScript::Property *prop,
                              QQmlScript::Object *obj,
                              const QQmlCompilerTypes::BindingContext &ctxt);
    bool buildValueTypeProperty(QObject *type, 
                                QQmlScript::Object *obj, 
                                QQmlScript::Object *baseObj,
                                const QQmlCompilerTypes::BindingContext &ctxt);
    bool buildListProperty(QQmlScript::Property *prop,
                           QQmlScript::Object *obj,
                           const QQmlCompilerTypes::BindingContext &ctxt);
    bool buildScriptStringProperty(QQmlScript::Property *prop,
                                   QQmlScript::Object *obj,
                                   const QQmlCompilerTypes::BindingContext &ctxt);
    bool buildPropertyAssignment(QQmlScript::Property *prop,
                                 QQmlScript::Object *obj,
                                 const QQmlCompilerTypes::BindingContext &ctxt);
    bool buildPropertyObjectAssignment(QQmlScript::Property *prop,
                                       QQmlScript::Object *obj,
                                       QQmlScript::Value *value,
                                       const QQmlCompilerTypes::BindingContext &ctxt);
    bool buildPropertyOnAssignment(QQmlScript::Property *prop,
                                   QQmlScript::Object *obj,
                                   QQmlScript::Object *baseObj,
                                   QQmlScript::Value *value,
                                   const QQmlCompilerTypes::BindingContext &ctxt);
    bool buildPropertyLiteralAssignment(QQmlScript::Property *prop,
                                        QQmlScript::Object *obj,
                                        QQmlScript::Value *value,
                                        const QQmlCompilerTypes::BindingContext &ctxt);
    bool doesPropertyExist(QQmlScript::Property *prop, QQmlScript::Object *obj);
    bool testLiteralAssignment(QQmlScript::Property *prop,
                               QQmlScript::Value *value);
    bool testQualifiedEnumAssignment(QQmlScript::Property *prop,
                                     QQmlScript::Object *obj,
                                     QQmlScript::Value *value,
                                     bool *isAssignment);
    enum DynamicMetaMode { Normal, ForceCreation };
    bool mergeDynamicMetaProperties(QQmlScript::Object *obj);
    bool buildDynamicMeta(QQmlScript::Object *obj, DynamicMetaMode mode);
    bool buildDynamicMetaAliases(QQmlScript::Object *obj);
    bool checkDynamicMeta(QQmlScript::Object *obj);
    bool buildBinding(QQmlScript::Value *, QQmlScript::Property *prop,
                      const QQmlCompilerTypes::BindingContext &ctxt);
    bool buildLiteralBinding(QQmlScript::Value *, QQmlScript::Property *prop,
                             const QQmlCompilerTypes::BindingContext &ctxt);
    bool buildComponentFromRoot(QQmlScript::Object *obj, const QQmlCompilerTypes::BindingContext &);
    bool completeComponentBuild();
    bool checkValidId(QQmlScript::Value *, const QString &);


    void genObject(QQmlScript::Object *obj, bool parentToSuper = false);
    void genObjectBody(QQmlScript::Object *obj);
    void genValueTypeProperty(QQmlScript::Object *obj,QQmlScript::Property *);
    void genComponent(QQmlScript::Object *obj);
    void genValueProperty(QQmlScript::Property *prop, QQmlScript::Object *obj);
    void genListProperty(QQmlScript::Property *prop, QQmlScript::Object *obj);
    void genPropertyAssignment(QQmlScript::Property *prop, 
                               QQmlScript::Object *obj,
                               QQmlScript::Property *valueTypeProperty = 0);
    void genLiteralAssignment(QQmlScript::Property *prop,
                              QQmlScript::Value *value);
    void genBindingAssignment(QQmlScript::Value *binding, 
                              QQmlScript::Property *prop, 
                              QQmlScript::Object *obj,
                              QQmlScript::Property *valueTypeProperty = 0);
    int genContextCache();

    QQmlPropertyData genValueTypeData(QQmlScript::Property *prop,
                                              QQmlScript::Property *valueTypeProp);

    int componentTypeRef();
    int translationContextIndex();

    QQmlType *toQmlType(QQmlScript::Object *from);
    bool canCoerce(int to, QQmlScript::Object *from);

    QString elementName(QQmlScript::Object *);

    QStringList deferredProperties(QQmlScript::Object *);

    QQmlPropertyCache *propertyCacheForObject(QQmlScript::Object *);
    QQmlPropertyData *property(QQmlScript::Object *, int);
    QQmlPropertyData *property(QQmlScript::Object *, const QHashedStringRef &,
                                       bool *notInRevision = 0);
    QQmlPropertyData *signal(QQmlScript::Object *, const QHashedStringRef &,
                                     bool *notInRevision = 0);
    int indexOfProperty(QQmlScript::Object *, const QHashedStringRef &, bool *notInRevision = 0);
    int indexOfProperty(QQmlScript::Object *, const QString &, bool *notInRevision = 0);
    int indexOfSignal(QQmlScript::Object *, const QString &, bool *notInRevision = 0);

    void addId(const QString &, QQmlScript::Object *);

    void dumpStats();

    void addBindingReference(QQmlCompilerTypes::JSBindingReference *);

    QQmlCompilerTypes::ComponentCompileState *compileState;

    QQmlPool *pool;

    QQmlCompilerTypes::ComponentCompileState *componentState(QQmlScript::Object *);
    void saveComponentState();

    QList<QQmlError> exceptions;
    QQmlCompiledData *output;
    QQmlEngine *engine;
    QQmlEnginePrivate *enginePrivate;
    QQmlScript::Object *unitRoot;
    QQmlTypeData *unit;
    int cachedComponentTypeRef;
    int cachedTranslationContextIndex;

    // Compiler component statistics.  Only collected if QML_COMPILER_STATS=1
    struct ComponentStat
    {
        ComponentStat() : lineNumber(0), ids(0), objects(0) {}

        quint16 lineNumber;

        int ids;
        QList<QQmlScript::LocationSpan> scriptBindings;
        QList<QQmlScript::LocationSpan> sharedBindings;
        QList<QQmlScript::LocationSpan> optimizedBindings;
        int objects;
    };
    struct ComponentStats : public QQmlPool::Class
    {
        ComponentStat componentStat;
        QList<ComponentStat> savedComponentStats;
    };
    ComponentStats *componentStats;
};

QT_END_NAMESPACE

#endif // QQMLCOMPILER_P_H
