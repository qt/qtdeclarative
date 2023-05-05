/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the tools applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
******************************************************************************/
#ifndef QQMLOBJECTCREATOR_P_H
#define QQMLOBJECTCREATOR_P_H

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

#include <private/qqmlimport_p.h>
#include <private/qqmltypenamecache_p.h>
#include <private/qv4compileddata_p.h>
#include <private/qfinitestack_p.h>
#include <private/qrecursionwatcher_p.h>
#include <private/qqmlprofiler_p.h>
#include <private/qv4qmlcontext_p.h>
#include <private/qqmlguardedcontextdata_p.h>

#include <qpointer.h>

QT_BEGIN_NAMESPACE

class QQmlAbstractBinding;
class QQmlInstantiationInterrupt;
class QQmlIncubatorPrivate;

struct AliasToRequiredInfo {
    QString propertyName;
    QUrl fileUrl;
};

/*!
\internal
This struct contains information solely used for displaying error messages
\variable aliasesToRequired allows us to give the user a way to know which (aliasing) properties
can be set to set the required property
\sa QQmlComponentPrivate::unsetRequiredPropertyToQQmlError
*/
struct RequiredPropertyInfo
{
    QString propertyName;
    QUrl fileUrl;
    QV4::CompiledData::Location location;
    QVector<AliasToRequiredInfo> aliasesToRequired;
};

class RequiredProperties : public QHash<QQmlPropertyData*, RequiredPropertyInfo> {};

struct DeferredQPropertyBinding {
    QObject *target = nullptr;
    int properyIndex = -1;
    QUntypedPropertyBinding binding;
};

struct QQmlObjectCreatorSharedState : QQmlRefCount
{
    QQmlRefPointer<QQmlContextData> rootContext;
    QQmlRefPointer<QQmlContextData> creationContext;
    QFiniteStack<QQmlAbstractBinding::Ptr> allCreatedBindings;
    QFiniteStack<QQmlParserStatus*> allParserStatusCallbacks;
    QFiniteStack<QQmlGuard<QObject> > allCreatedObjects;
    QV4::Value *allJavaScriptObjects; // pointer to vector on JS stack to reference JS wrappers during creation phase.
    QQmlComponentAttached *componentAttached;
    QList<QQmlEnginePrivate::FinalizeCallback> finalizeCallbacks;
    QQmlVmeProfiler profiler;
    QRecursionNode recursionNode;
    RequiredProperties requiredProperties;
    QList<DeferredQPropertyBinding> allQPropertyBindings;
    bool hadRequiredProperties;
};

class Q_QML_PRIVATE_EXPORT QQmlObjectCreator
{
    Q_DECLARE_TR_FUNCTIONS(QQmlObjectCreator)
public:
    QQmlObjectCreator(QQmlRefPointer<QQmlContextData> parentContext,
                      const QQmlRefPointer<QV4::ExecutableCompilationUnit> &compilationUnit,
                      const QQmlRefPointer<QQmlContextData> &creationContext,
                      QQmlIncubatorPrivate  *incubator = nullptr);
    ~QQmlObjectCreator();

    enum CreationFlags { NormalObject = 1, InlineComponent = 2 };
    QObject *create(int subComponentIndex = -1, QObject *parent = nullptr,
                    QQmlInstantiationInterrupt *interrupt = nullptr, int flags = NormalObject);

    bool populateDeferredProperties(QObject *instance, const QQmlData::DeferredData *deferredData);

    void beginPopulateDeferred(const QQmlRefPointer<QQmlContextData> &context);
    void populateDeferredBinding(const QQmlProperty &qmlProperty, int deferredIndex,
                                 const QV4::CompiledData::Binding *binding);
    void finalizePopulateDeferred();

    bool finalize(QQmlInstantiationInterrupt &interrupt);
    void clear();

    QQmlRefPointer<QQmlContextData> rootContext() const { return sharedState->rootContext; }
    QQmlComponentAttached **componentAttachment() { return &sharedState->componentAttached; }

    QList<QQmlEnginePrivate::FinalizeCallback> *finalizeCallbacks() { return &sharedState->finalizeCallbacks; }

    QList<QQmlError> errors;

    QQmlRefPointer<QQmlContextData> parentContextData() const
    {
        return parentContext.contextData();
    }
    QFiniteStack<QQmlGuard<QObject> > &allCreatedObjects() { return sharedState->allCreatedObjects; }

    RequiredProperties &requiredProperties() {return sharedState->requiredProperties;}
    bool componentHadRequiredProperties() const {return sharedState->hadRequiredProperties;}

    void removePendingBinding(QObject *target, int propertyIndex)
    {
        QList<DeferredQPropertyBinding> &pendingBindings = sharedState.data()->allQPropertyBindings;
        auto it = std::remove_if(pendingBindings.begin(), pendingBindings.end(),
                                 [&](const DeferredQPropertyBinding &deferred) {
            return deferred.properyIndex == propertyIndex && deferred.target == target;
        });
        pendingBindings.erase(it, pendingBindings.end());
    }

private:
    QQmlObjectCreator(QQmlRefPointer<QQmlContextData> contextData,
                      const QQmlRefPointer<QV4::ExecutableCompilationUnit> &compilationUnit,
                      QQmlObjectCreatorSharedState *inheritedSharedState);

    void init(QQmlRefPointer<QQmlContextData> parentContext);

    QObject *createInstance(int index, QObject *parent = nullptr, bool isContextObject = false);

    bool populateInstance(int index, QObject *instance, QObject *bindingTarget,
                          const QQmlPropertyData *valueTypeProperty,
                          const QV4::CompiledData::Binding *binding = nullptr);

    // If qmlProperty and binding are null, populate all properties, otherwise only the given one.
    void populateDeferred(QObject *instance, int deferredIndex,
                          const QQmlPropertyPrivate *qmlProperty = nullptr,
                          const QV4::CompiledData::Binding *binding = nullptr);

    void setupBindings(bool applyDeferredBindings = false);
    bool setPropertyBinding(const QQmlPropertyData *property, const QV4::CompiledData::Binding *binding);
    void setPropertyValue(const QQmlPropertyData *property, const QV4::CompiledData::Binding *binding);
    void setupFunctions();

    QString stringAt(int idx) const { return compilationUnit->stringAt(idx); }
    void recordError(const QV4::CompiledData::Location &location, const QString &description);

    void registerObjectWithContextById(const QV4::CompiledData::Object *object, QObject *instance) const;

    inline QV4::QmlContext *currentQmlContext();
    QV4::ResolvedTypeReference *resolvedType(int id) const
    {
        return compilationUnit->resolvedType(id);
    }

    enum Phase {
        Startup,
        CreatingObjects,
        CreatingObjectsPhase2,
        ObjectsCreated,
        Finalizing,
        Done
    } phase;

    QQmlEngine *engine;
    QV4::ExecutionEngine *v4;
    QQmlRefPointer<QV4::ExecutableCompilationUnit> compilationUnit;
    const QV4::CompiledData::Unit *qmlUnit;
    QQmlGuardedContextData parentContext;
    QQmlRefPointer<QQmlContextData> context;
    const QQmlPropertyCacheVector *propertyCaches;
    QQmlRefPointer<QQmlObjectCreatorSharedState> sharedState;
    bool topLevelCreator;
    QQmlIncubatorPrivate *incubator;

    QObject *_qobject;
    QObject *_scopeObject;
    QObject *_bindingTarget;

    const QQmlPropertyData *_valueTypeProperty; // belongs to _qobjectForBindings's property cache
    int _compiledObjectIndex;
    const QV4::CompiledData::Object *_compiledObject;
    QQmlData *_ddata;
    QQmlRefPointer<QQmlPropertyCache> _propertyCache;
    QQmlVMEMetaObject *_vmeMetaObject;
    QQmlListProperty<void> _currentList;
    QV4::QmlContext *_qmlContext;

    friend struct QQmlObjectCreatorRecursionWatcher;

    typedef std::function<bool(QQmlObjectCreatorSharedState *sharedState)> PendingAliasBinding;
    std::vector<PendingAliasBinding> pendingAliasBindings;
};

struct QQmlObjectCreatorRecursionWatcher
{
    QQmlObjectCreatorRecursionWatcher(QQmlObjectCreator *creator);

    bool hasRecursed() const { return watcher.hasRecursed(); }

private:
    QQmlRefPointer<QQmlObjectCreatorSharedState> sharedState;
    QRecursionWatcher<QQmlObjectCreatorSharedState, &QQmlObjectCreatorSharedState::recursionNode> watcher;
};

QV4::QmlContext *QQmlObjectCreator::currentQmlContext()
{
    if (!_qmlContext->isManaged())
        _qmlContext->setM(QV4::QmlContext::create(v4->rootContext(), context, _scopeObject));

    return _qmlContext;
}

QT_END_NAMESPACE

#endif // QQMLOBJECTCREATOR_P_H
