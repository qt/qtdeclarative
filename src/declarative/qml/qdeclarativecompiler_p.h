/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
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
#include "private/qv8_p.h"
#include "private/qdeclarativeinstruction_p.h"
#include "private/qdeclarativeparser_p.h"
#include "private/qdeclarativeengine_p.h"
#include "private/qbitfield_p.h"
#include "private/qdeclarativepropertycache_p.h"
#include "private/qdeclarativeintegercache_p.h"
#include "private/qdeclarativetypenamecache_p.h"
#include "private/qdeclarativetypeloader_p.h"

#include <QtCore/qbytearray.h>
#include <QtCore/qset.h>
#include <QtCore/QCoreApplication>

QT_BEGIN_NAMESPACE

class QDeclarativeEngine;
class QDeclarativeComponent;
class QDeclarativeContext;
class QDeclarativeContextData;

class Q_AUTOTEST_EXPORT QDeclarativeCompiledData : public QDeclarativeRefCount, public QDeclarativeCleanup
{
public:
    QDeclarativeCompiledData(QDeclarativeEngine *engine);
    virtual ~QDeclarativeCompiledData();

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

        QObject *createInstance(QDeclarativeContextData *, const QBitField &, QList<QDeclarativeError> *) const;
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
    QList<QJSValue *> cachedClosures;
    QList<QDeclarativePropertyCache *> propertyCaches;
    QList<QDeclarativeIntegerCache *> contextCaches;
    QList<QDeclarativeScriptData *> scripts;
    QList<QUrl> urls;

    void dumpInstructions();

    int addInstruction(const QDeclarativeInstruction &instr);
    int nextInstructionIndex();
    QDeclarativeInstruction *instruction(int index);

protected:
    virtual void clear(); // From QDeclarativeCleanup

private:
    void dump(QDeclarativeInstruction *, int idx = -1);
    QDeclarativeCompiledData(const QDeclarativeCompiledData &other);
    QDeclarativeCompiledData &operator=(const QDeclarativeCompiledData &other);
    QByteArray packData;
    friend class QDeclarativeCompiler;
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
        BindingContext(QDeclarativeParser::Object *o)
            : stack(0), owner(0), object(o) {}
        BindingContext incr() const {
            BindingContext rv(object);
            rv.stack = stack + 1;
            return rv;
        }
        bool isSubContext() const { return stack != 0; }
        int stack;
        int owner;
        QDeclarativeParser::Object *object;
    };

    struct BindingReference : public QDeclarativePool::Class 
    {
        BindingReference() : nextReference(0) {}

        QDeclarativeParser::Variant expression;
        QDeclarativeParser::Property *property;
        QDeclarativeParser::Value *value;

        enum DataType { QtScript, V4, V8 };
        DataType dataType;

        int compiledIndex;

        QString rewrittenExpression;
        BindingContext bindingContext;

        BindingReference *nextReference;
    };

    // Contains all the incremental compiler state about a component.  As
    // a single QML file can have multiple components defined, there may be
    // more than one of these for each compile
    struct ComponentCompileState : public QDeclarativePool::Class
    {
        ComponentCompileState() 
            : parserStatusCount(0), pushedProperties(0), nested(false), v8BindingProgramLine(-1), root(0) {}
        QHash<QString, QDeclarativeParser::Object *> ids;
        int parserStatusCount;
        int pushedProperties;
        bool nested;

        QByteArray compiledBindingData;
        QString v8BindingProgram;
        int v8BindingProgramLine;
        int v8BindingProgramIndex;

        struct BindingReferenceList {
            BindingReferenceList() : _count(0), _first(0) {}
            QDeclarativeCompilerTypes::BindingReference *first() const { return _first; }
            void prepend(QDeclarativeCompilerTypes::BindingReference *ref) {
                Q_ASSERT(ref);
                Q_ASSERT(0 == ref->nextReference);
                ref->nextReference = _first;
                _first = ref;
                ++_count;
            }
            int count() const { return _count; }
        private:
            int _count;
            QDeclarativeCompilerTypes::BindingReference *_first;
        };

        BindingReferenceList bindings;
        QHash<QDeclarativeParser::Value *, QDeclarativeCompilerTypes::BindingContext> signalExpressions;
        QList<QDeclarativeParser::Object *> aliasingObjects;
        QDeclarativeParser::Object *root;
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

    int evaluateEnum(const QByteArray& script) const; // for QDeclarativeCustomParser::evaluateEnum
    const QMetaObject *resolveType(const QByteArray& name) const; // for QDeclarativeCustomParser::resolveType
    int rewriteBinding(const QString& expression, const QString& name); // for QDeclarativeCustomParser::rewriteBinding

private:
    static void reset(QDeclarativeCompiledData *);

    void compileTree(QDeclarativeParser::Object *tree);


    bool buildObject(QDeclarativeParser::Object *obj, const QDeclarativeCompilerTypes::BindingContext &);
    bool buildComponent(QDeclarativeParser::Object *obj, const QDeclarativeCompilerTypes::BindingContext &);
    bool buildSubObject(QDeclarativeParser::Object *obj, const QDeclarativeCompilerTypes::BindingContext &);
    bool buildSignal(QDeclarativeParser::Property *prop, QDeclarativeParser::Object *obj, 
                     const QDeclarativeCompilerTypes::BindingContext &);
    bool buildProperty(QDeclarativeParser::Property *prop, QDeclarativeParser::Object *obj, 
                       const QDeclarativeCompilerTypes::BindingContext &);
    bool buildPropertyInNamespace(QDeclarativeImportedNamespace *ns,
                                  QDeclarativeParser::Property *prop, 
                                  QDeclarativeParser::Object *obj, 
                                  const QDeclarativeCompilerTypes::BindingContext &);
    bool buildIdProperty(QDeclarativeParser::Property *prop, QDeclarativeParser::Object *obj);
    bool buildAttachedProperty(QDeclarativeParser::Property *prop, 
                               QDeclarativeParser::Object *obj,
                               const QDeclarativeCompilerTypes::BindingContext &ctxt);
    bool buildGroupedProperty(QDeclarativeParser::Property *prop,
                              QDeclarativeParser::Object *obj,
                              const QDeclarativeCompilerTypes::BindingContext &ctxt);
    bool buildValueTypeProperty(QObject *type, 
                                QDeclarativeParser::Object *obj, 
                                QDeclarativeParser::Object *baseObj,
                                const QDeclarativeCompilerTypes::BindingContext &ctxt);
    bool buildListProperty(QDeclarativeParser::Property *prop,
                           QDeclarativeParser::Object *obj,
                           const QDeclarativeCompilerTypes::BindingContext &ctxt);
    bool buildScriptStringProperty(QDeclarativeParser::Property *prop,
                                   QDeclarativeParser::Object *obj,
                                   const QDeclarativeCompilerTypes::BindingContext &ctxt);
    bool buildPropertyAssignment(QDeclarativeParser::Property *prop,
                                 QDeclarativeParser::Object *obj,
                                 const QDeclarativeCompilerTypes::BindingContext &ctxt);
    bool buildPropertyObjectAssignment(QDeclarativeParser::Property *prop,
                                       QDeclarativeParser::Object *obj,
                                       QDeclarativeParser::Value *value,
                                       const QDeclarativeCompilerTypes::BindingContext &ctxt);
    bool buildPropertyOnAssignment(QDeclarativeParser::Property *prop,
                                   QDeclarativeParser::Object *obj,
                                   QDeclarativeParser::Object *baseObj,
                                   QDeclarativeParser::Value *value,
                                   const QDeclarativeCompilerTypes::BindingContext &ctxt);
    bool buildPropertyLiteralAssignment(QDeclarativeParser::Property *prop,
                                        QDeclarativeParser::Object *obj,
                                        QDeclarativeParser::Value *value,
                                        const QDeclarativeCompilerTypes::BindingContext &ctxt);
    bool doesPropertyExist(QDeclarativeParser::Property *prop, QDeclarativeParser::Object *obj);
    bool testLiteralAssignment(const QMetaProperty &prop, 
                               QDeclarativeParser::Value *value);
    bool testQualifiedEnumAssignment(const QMetaProperty &prop,
                                     QDeclarativeParser::Object *obj,
                                     QDeclarativeParser::Value *value,
                                     bool *isAssignment);
    enum DynamicMetaMode { IgnoreAliases, ResolveAliases, ForceCreation };
    bool mergeDynamicMetaProperties(QDeclarativeParser::Object *obj);
    bool buildDynamicMeta(QDeclarativeParser::Object *obj, DynamicMetaMode mode);
    bool checkDynamicMeta(QDeclarativeParser::Object *obj);
    bool buildBinding(QDeclarativeParser::Value *, QDeclarativeParser::Property *prop,
                      const QDeclarativeCompilerTypes::BindingContext &ctxt);
    bool buildComponentFromRoot(QDeclarativeParser::Object *obj, const QDeclarativeCompilerTypes::BindingContext &);
    bool compileAlias(QMetaObjectBuilder &, 
                      QByteArray &data,
                      QDeclarativeParser::Object *obj, 
                      const QDeclarativeParser::Object::DynamicProperty &);
    bool completeComponentBuild();
    bool checkValidId(QDeclarativeParser::Value *, const QString &);


    void genObject(QDeclarativeParser::Object *obj);
    void genObjectBody(QDeclarativeParser::Object *obj);
    void genValueTypeProperty(QDeclarativeParser::Object *obj,QDeclarativeParser::Property *);
    void genComponent(QDeclarativeParser::Object *obj);
    void genValueProperty(QDeclarativeParser::Property *prop, QDeclarativeParser::Object *obj);
    void genListProperty(QDeclarativeParser::Property *prop, QDeclarativeParser::Object *obj);
    void genPropertyAssignment(QDeclarativeParser::Property *prop, 
                               QDeclarativeParser::Object *obj,
                               QDeclarativeParser::Property *valueTypeProperty = 0);
    void genLiteralAssignment(const QMetaProperty &prop, 
                              QDeclarativeParser::Value *value);
    void genBindingAssignment(QDeclarativeParser::Value *binding, 
                              QDeclarativeParser::Property *prop, 
                              QDeclarativeParser::Object *obj,
                              QDeclarativeParser::Property *valueTypeProperty = 0);
    int genContextCache();

    int genValueTypeData(QDeclarativeParser::Property *prop, QDeclarativeParser::Property *valueTypeProp);
    int genPropertyData(QDeclarativeParser::Property *prop);

    int componentTypeRef();

    static QDeclarativeType *toQmlType(QDeclarativeParser::Object *from);
    bool canCoerce(int to, QDeclarativeParser::Object *from);

    QString elementName(QDeclarativeParser::Object *);

    QStringList deferredProperties(QDeclarativeParser::Object *);
    int indexOfProperty(QDeclarativeParser::Object *, const QString &, bool *notInRevision = 0);
    int indexOfSignal(QDeclarativeParser::Object *, const QString &, bool *notInRevision = 0);

    void addId(const QString &, QDeclarativeParser::Object *);

    void dumpStats();

    void addBindingReference(QDeclarativeCompilerTypes::BindingReference *);

    QDeclarativeCompilerTypes::ComponentCompileState *compileState;

    QDeclarativePool *pool;

    QDeclarativeCompilerTypes::ComponentCompileState *componentState(QDeclarativeParser::Object *);
    void saveComponentState();

    QList<QDeclarativeError> exceptions;
    QDeclarativeCompiledData *output;
    QDeclarativeEngine *engine;
    QDeclarativeEnginePrivate *enginePrivate;
    QDeclarativeParser::Object *unitRoot;
    QDeclarativeTypeData *unit;


    // Compiler component statistics.  Only collected if QML_COMPILER_STATS=1
    struct ComponentStat
    {
        ComponentStat() : ids(0), objects(0) {}

        int lineNumber;

        int ids;
        QList<QDeclarativeParser::LocationSpan> scriptBindings;
        QList<QDeclarativeParser::LocationSpan> optimizedBindings;
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
