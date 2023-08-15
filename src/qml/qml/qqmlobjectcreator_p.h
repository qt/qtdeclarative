// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
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
#include <private/qqmlfinalizer_p.h>
#include <private/qqmlvmemetaobject_p.h>

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

struct RequiredPropertyKey
{
    RequiredPropertyKey() = default;
    RequiredPropertyKey(const QObject *object, const QQmlPropertyData *data)
        : object(object)
        , data(data)
    {}

    const QObject *object = nullptr;
    const QQmlPropertyData *data = nullptr;

private:
    friend size_t qHash(const RequiredPropertyKey &key, size_t seed = 0)
    {
        return qHashMulti(seed, key.object, key.data);
    }

    friend bool operator==(const RequiredPropertyKey &a, const RequiredPropertyKey &b)
    {
        return a.object == b.object && a.data == b.data;
    }
};

class RequiredProperties : public QHash<RequiredPropertyKey, RequiredPropertyInfo> {};

struct DeferredQPropertyBinding {
    QObject *target = nullptr;
    int properyIndex = -1;
    QUntypedPropertyBinding binding;
};

struct QQmlObjectCreatorSharedState final : QQmlRefCounted<QQmlObjectCreatorSharedState>
{
    QQmlRefPointer<QQmlContextData> rootContext;
    QQmlRefPointer<QQmlContextData> creationContext;
    QFiniteStack<QQmlAbstractBinding::Ptr> allCreatedBindings;
    QFiniteStack<QQmlParserStatus*> allParserStatusCallbacks;
    QFiniteStack<QQmlGuard<QObject> > allCreatedObjects;
    QV4::Value *allJavaScriptObjects; // pointer to vector on JS stack to reference JS wrappers during creation phase.
    QQmlComponentAttached *componentAttached;
    QList<QQmlFinalizerHook *> finalizeHooks;
    QQmlVmeProfiler profiler;
    QRecursionNode recursionNode;
    RequiredProperties requiredProperties;
    QList<DeferredQPropertyBinding> allQPropertyBindings;
    bool hadTopLevelRequiredProperties;
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
    void populateDeferredInstance(QObject *outerObject, int deferredIndex,
                                  int index, QObject *instance, QObject *bindingTarget,
                                  const QQmlPropertyData *valueTypeProperty,
                                  const QV4::CompiledData::Binding *binding = nullptr);
    void finalizePopulateDeferred();

    bool finalize(QQmlInstantiationInterrupt &interrupt);
    void clear();

    QQmlRefPointer<QQmlContextData> rootContext() const { return sharedState->rootContext; }
    QQmlComponentAttached **componentAttachment() { return &sharedState->componentAttached; }

    QList<QQmlError> errors;

    QQmlRefPointer<QQmlContextData> parentContextData() const
    {
        return parentContext.contextData();
    }
    QFiniteStack<QQmlGuard<QObject> > &allCreatedObjects() { return sharedState->allCreatedObjects; }

    RequiredProperties *requiredProperties() {return &sharedState->requiredProperties;}
    bool componentHadTopLevelRequiredProperties() const {return sharedState->hadTopLevelRequiredProperties;}

    static QQmlComponent *createComponent(QQmlEngine *engine,
                                          QV4::ExecutableCompilationUnit *compilationUnit,
                                          int index, QObject *parent,
                                          const QQmlRefPointer<QQmlContextData> &context);

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
                      QQmlObjectCreatorSharedState *inheritedSharedState,
                      bool isContextObject);

    void init(QQmlRefPointer<QQmlContextData> parentContext);

    QObject *createInstance(int index, QObject *parent = nullptr, bool isContextObject = false);

    bool populateInstance(int index, QObject *instance, QObject *bindingTarget,
                          const QQmlPropertyData *valueTypeProperty,
                          const QV4::CompiledData::Binding *binding = nullptr);

    // If qmlProperty and binding are null, populate all properties, otherwise only the given one.
    void populateDeferred(QObject *instance, int deferredIndex);
    void populateDeferred(QObject *instance, int deferredIndex,
                          const QQmlPropertyPrivate *qmlProperty,
                          const QV4::CompiledData::Binding *binding);

    enum BindingMode {
        ApplyNone      = 0x0,
        ApplyImmediate = 0x1,
        ApplyDeferred  = 0x2,
        ApplyAll       = ApplyImmediate | ApplyDeferred,
    };
    Q_DECLARE_FLAGS(BindingSetupFlags, BindingMode);

    void setupBindings(BindingSetupFlags mode = BindingMode::ApplyImmediate);
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
    bool isContextObject;
    QQmlIncubatorPrivate *incubator;

    QObject *_qobject;
    QObject *_scopeObject;
    QObject *_bindingTarget;

    const QQmlPropertyData *_valueTypeProperty; // belongs to _qobjectForBindings's property cache
    int _compiledObjectIndex;
    const QV4::CompiledData::Object *_compiledObject;
    QQmlData *_ddata;
    QQmlPropertyCache::ConstPtr _propertyCache;
    QQmlVMEMetaObject *_vmeMetaObject;
    QQmlListProperty<void> _currentList;
    QV4::QmlContext *_qmlContext;

    friend struct QQmlObjectCreatorRecursionWatcher;

    typedef std::function<bool(QQmlObjectCreatorSharedState *sharedState)> PendingAliasBinding;
    std::vector<PendingAliasBinding> pendingAliasBindings;

    template<typename Functor>
    void doPopulateDeferred(QObject *instance, int deferredIndex, Functor f)
    {
        QQmlData *declarativeData = QQmlData::get(instance);
        QObject *bindingTarget = instance;

        QQmlPropertyCache::ConstPtr cache = declarativeData->propertyCache;
        QQmlVMEMetaObject *vmeMetaObject = QQmlVMEMetaObject::get(instance);

        QObject *scopeObject = instance;
        qt_ptr_swap(_scopeObject, scopeObject);

        QV4::Scope valueScope(v4);
        QScopedValueRollback<QV4::Value*> jsObjectGuard(sharedState->allJavaScriptObjects,
                                                        valueScope.alloc(compilationUnit->totalObjectCount()));

        Q_ASSERT(topLevelCreator);
        QV4::QmlContext *qmlContext = static_cast<QV4::QmlContext *>(valueScope.alloc());

        qt_ptr_swap(_qmlContext, qmlContext);

        _propertyCache.swap(cache);
        qt_ptr_swap(_qobject, instance);

        int objectIndex = deferredIndex;
        std::swap(_compiledObjectIndex, objectIndex);

        const QV4::CompiledData::Object *obj = compilationUnit->objectAt(_compiledObjectIndex);
        qt_ptr_swap(_compiledObject, obj);
        qt_ptr_swap(_ddata, declarativeData);
        qt_ptr_swap(_bindingTarget, bindingTarget);
        qt_ptr_swap(_vmeMetaObject, vmeMetaObject);

        f();

        qt_ptr_swap(_vmeMetaObject, vmeMetaObject);
        qt_ptr_swap(_bindingTarget, bindingTarget);
        qt_ptr_swap(_ddata, declarativeData);
        qt_ptr_swap(_compiledObject, obj);
        std::swap(_compiledObjectIndex, objectIndex);
        qt_ptr_swap(_qobject, instance);
        _propertyCache.swap(cache);

        qt_ptr_swap(_qmlContext, qmlContext);
        qt_ptr_swap(_scopeObject, scopeObject);
    }
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
