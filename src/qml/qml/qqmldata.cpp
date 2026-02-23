// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#include "qqmldata_p.h"

#include <private/qmetaobject_p.h>
#include <private/qqmlabstractbinding_p.h>
#include <private/qqmlboundsignal_p.h>
#include <private/qqmlcontextdata_p.h>
#include <private/qqmlnotifier_p.h>

#include <QtCore/qtyperevision.h>
#include <private/qthread_p.h>

QT_BEGIN_NAMESPACE

QQmlData::QQmlData(Ownership ownership)
    : ownMemory(ownership == OwnsMemory)
    , indestructible(true)
    , explicitIndestructibleSet(false)
    , hasTaintedV4Object(false)
    , isQueuedForDeletion(false)
    , rootObjectInCreation(false)
    , hasInterceptorMetaObject(false)
    , hasVMEMetaObject(false)
    , hasConstWrapper(false)
    , dummy(0)
    , bindingBitsArraySize(InlineBindingArraySize)
{
    memset(bindingBitsValue, 0, sizeof(bindingBitsValue));
    init();
}

QQmlData::~QQmlData() = default;

void QQmlData::destroyed(QAbstractDeclarativeData *d, QObject *o)
{
    QQmlData *ddata = static_cast<QQmlData *>(d);
    ddata->destroyed(o);
}

class QQmlThreadNotifierProxyObject : public QObject
{
public:
    QPointer<QObject> target;

    int qt_metacall(QMetaObject::Call, int methodIndex, void **a) override {
        if (!target)
            return -1;

        QMetaMethod method = target->metaObject()->method(methodIndex);
        Q_ASSERT(method.methodType() == QMetaMethod::Signal);
        int signalIndex = QMetaObjectPrivate::signalIndex(method);
        QQmlData *ddata = QQmlData::get(target, false);
        QQmlNotifierEndpoint *ep = ddata->notify(signalIndex);
        if (ep) QQmlNotifier::emitNotify(ep, a);

        delete this;

        return -1;
    }
};

void QQmlData::signalEmitted(QAbstractDeclarativeData *, QObject *object, int index, void **a)
{
    QQmlData *ddata = QQmlData::get(object, false);
    if (!ddata) return; // Probably being deleted

    // In general, QML only supports QObject's that live on the same thread as the QQmlEngine
    // that they're exposed to.  However, to make writing "worker objects" that calculate data
    // in a separate thread easier, QML allows a QObject that lives in the same thread as the
    // QQmlEngine to emit signals from a different thread.  These signals are then automatically
    // marshalled back onto the QObject's thread and handled by QML from there.  This is tested
    // by the qqmlecmascript::threadSignal() autotest.

    // Relaxed semantics here. If we're on a different thread we might schedule a useless event,
    // but that should be rare.
    if (!ddata->notifyList.loadRelaxed())
        return;

    auto objectThreadData = QObjectPrivate::get(object)->threadData.loadRelaxed();
    if (QThread::currentThreadId() != objectThreadData->threadId.loadRelaxed()) {
        if (!objectThreadData->thread.loadAcquire())
            return;

        QMetaMethod m = QMetaObjectPrivate::signal(object->metaObject(), index);
        const QList<QByteArray> parameterTypes = m.parameterTypes();

        QVarLengthArray<const QtPrivate::QMetaTypeInterface *, 16> argTypes;
        argTypes.reserve(1 + parameterTypes.size());
        argTypes.emplace_back(nullptr); // return type
        for (const QByteArray &typeName: parameterTypes) {
            QMetaType type;
            if (typeName.endsWith('*'))
                type = QMetaType(QMetaType::VoidStar);
            else
                type = QMetaType::fromName(typeName);

            if (!type.isValid()) {
                qWarning("QObject::connect: Cannot queue arguments of type '%s'\n"
                         "(Make sure '%s' is registered using qRegisterMetaType().)",
                         typeName.constData(), typeName.constData());
                return;
            }

            argTypes.emplace_back(type.iface());
        }

        auto ev = std::make_unique<QQueuedMetaCallEvent>(m.methodIndex(), 0, nullptr, object, index,
                                                         argTypes.size(), argTypes.data(), a);

        QQmlThreadNotifierProxyObject *mpo = new QQmlThreadNotifierProxyObject;
        mpo->target = object;
        mpo->moveToThread(objectThreadData->thread.loadAcquire());
        QCoreApplication::postEvent(mpo, ev.release());

    } else {
        QQmlNotifierEndpoint *ep = ddata->notify(index);
        if (ep) QQmlNotifier::emitNotify(ep, a);
    }
}

int QQmlData::receivers(QAbstractDeclarativeData *d, const QObject *, int index)
{
    QQmlData *ddata = static_cast<QQmlData *>(d);
    return ddata->endpointCount(index);
}

bool QQmlData::isSignalConnected(QAbstractDeclarativeData *d, const QObject *, int index)
{
    QQmlData *ddata = static_cast<QQmlData *>(d);
    return ddata->signalHasEndpoint(index);
}

int QQmlData::endpointCount(int index)
{
    int count = 0;
    QQmlNotifierEndpoint *ep = notify(index);
    if (!ep)
        return count;
    ++count;
    while (ep->next) {
        ++count;
        ep = ep->next;
    }
    return count;
}

void QQmlData::markAsDeleted(QObject *o)
{
    QVarLengthArray<QObject *> workStack;
    workStack.push_back(o);
    while (!workStack.isEmpty()) {
        auto currentObject = workStack.last();
        workStack.pop_back();
        QQmlData::setQueuedForDeletion(currentObject);
        auto currentObjectPriv = QObjectPrivate::get(currentObject);
        for (QObject *child: std::as_const(currentObjectPriv->children))
            workStack.push_back(child);
    }
}

void QQmlData::setQueuedForDeletion(QObject *object)
{
    if (object) {
        if (QQmlData *ddata = QQmlData::get(object)) {
            if (ddata->ownContext) {
                Q_ASSERT(ddata->ownContext.data() == ddata->context);
                ddata->ownContext->deepClearContextObject(object);
                ddata->ownContext.reset();
                ddata->context = nullptr;
            }
            ddata->isQueuedForDeletion = true;

            // Disconnect the notifiers now - during object destruction this would be too late,
            // since the disconnect call wouldn't be able to call disconnectNotify(), as it isn't
            // possible to get the metaobject anymore.
            // Also, there is no point in evaluating bindings in order to set properties on
            // half-deleted objects.
            ddata->disconnectNotifiers(DeleteNotifyList::No);
        }
    }
}

void QQmlData::flushPendingBinding(int coreIndex)
{
    clearPendingBindingBit(coreIndex);

    // Find the binding
    QQmlAbstractBinding *b = bindings;
    while (b && (b->targetPropertyIndex().coreIndex() != coreIndex ||
                 b->targetPropertyIndex().hasValueTypeIndex()))
        b = b->nextBinding();

    if (b && b->targetPropertyIndex().coreIndex() == coreIndex &&
        !b->targetPropertyIndex().hasValueTypeIndex())
        b->setEnabled(true, QQmlPropertyData::BypassInterceptor |
                              QQmlPropertyData::DontRemoveBinding);
}

QQmlData::DeferredData::DeferredData() = default;
QQmlData::DeferredData::~DeferredData() = default;

class QQmlDataExtended
{
    Q_DISABLE_COPY_MOVE(QQmlDataExtended)
public:
    QQmlDataExtended() = default;
    ~QQmlDataExtended() = default;

    QHash<QQmlAttachedPropertiesFunc, QObject *> attachedProperties;
};

void QQmlData::NotifyList::layout(QQmlNotifierEndpoint *endpoint)
{
    // Add a temporary sentinel at beginning of list. This will be overwritten
    // when the end point is inserted into the notifies further down.
    endpoint->prev = nullptr;

    while (endpoint->next) {
        Q_ASSERT(reinterpret_cast<QQmlNotifierEndpoint *>(endpoint->next->prev) == endpoint);
        endpoint = endpoint->next;
    }

    while (endpoint) {
        QQmlNotifierEndpoint *ep = (QQmlNotifierEndpoint *) endpoint->prev;

        int index = endpoint->sourceSignal;
        index = qMin(index, 0xFFFF - 1);

        endpoint->next = notifies[index];
        if (endpoint->next) endpoint->next->prev = &endpoint->next;
        endpoint->prev = &notifies[index];
        notifies[index] = endpoint;

        endpoint = ep;
    }
}

void QQmlData::NotifyList::layout()
{
    Q_ASSERT(maximumTodoIndex >= notifiesSize);

    if (todo) {
        QQmlNotifierEndpoint **old = notifies;
        const int reallocSize = (maximumTodoIndex + 1) * sizeof(QQmlNotifierEndpoint*);
        notifies = (QQmlNotifierEndpoint**)realloc(notifies, reallocSize);
        const int memsetSize = (maximumTodoIndex - notifiesSize + 1) *
                sizeof(QQmlNotifierEndpoint*);
        memset(notifies + notifiesSize, 0, memsetSize);

        if (notifies != old) {
            for (int ii = 0; ii < notifiesSize; ++ii)
                if (notifies[ii])
                    notifies[ii]->prev = &notifies[ii];
        }

        notifiesSize = maximumTodoIndex + 1;

        layout(todo);
    }

    maximumTodoIndex = 0;
    todo = nullptr;
}

void QQmlData::deferData(
        int objectIndex, const QQmlRefPointer<QV4::ExecutableCompilationUnit> &compilationUnit,
        const QQmlRefPointer<QQmlContextData> &context, const QString &inlineComponentName)
{
    QQmlData::DeferredData *deferData = new QQmlData::DeferredData;
    deferData->deferredIdx = objectIndex;
    deferData->compilationUnit = compilationUnit;
    deferData->context = context;
    deferData->inlineComponentName = inlineComponentName;

    const QV4::CompiledData::Object *compiledObject = compilationUnit->objectAt(objectIndex);
    const QV4::CompiledData::BindingPropertyData *propertyData
            = compilationUnit->bindingPropertyDataPerObjectAt(objectIndex);

    const QV4::CompiledData::Binding *binding = compiledObject->bindingTable();
    for (quint32 i = 0; i < compiledObject->nBindings; ++i, ++binding) {
        const QQmlPropertyData *property = propertyData->at(i);
        if (binding->hasFlag(QV4::CompiledData::Binding::IsDeferredBinding))
            deferData->bindings.insert(property ? property->coreIndex() : -1, binding);
    }

    deferredData.append(deferData);
}

void QQmlData::releaseDeferredData()
{
    auto it = deferredData.begin();
    while (it != deferredData.end()) {
        DeferredData *deferData = *it;
        if (deferData->bindings.isEmpty()) {
            delete deferData;
            it = deferredData.erase(it);
        } else {
            ++it;
        }
    }
}

void QQmlData::addNotify(int index, QQmlNotifierEndpoint *endpoint)
{
    // Can only happen on "home" thread. We apply relaxed semantics when loading the atomics.

    NotifyList *list = notifyList.loadRelaxed();

    if (!list) {
        list = new NotifyList;
        // We don't really care when this change takes effect on other threads. The notifyList can
        // only become non-null once in the life time of a QQmlData. It becomes null again when the
        // underlying QObject is deleted. At that point any interaction with the QQmlData is UB
        // anyway. So, for all intents and purposese, the list becomes non-null once and then stays
        // non-null "forever". We can apply relaxed semantics.
        notifyList.storeRelaxed(list);
    }

    Q_ASSERT(!endpoint->isConnected());

    index = qMin(index, 0xFFFF - 1);

    // Likewise, we don't really care _when_ the change in the connectionMask is propagated to other
    // threads. Cross-thread event ordering is inherently nondeterministic. Therefore, when querying
    // the conenctionMask in the presence of concurrent modification, any result is correct.
    list->connectionMask.storeRelaxed(
            list->connectionMask.loadRelaxed() | (1ULL << quint64(index % 64)));

    if (index < list->notifiesSize) {
        endpoint->next = list->notifies[index];
        if (endpoint->next) endpoint->next->prev = &endpoint->next;
        endpoint->prev = &list->notifies[index];
        list->notifies[index] = endpoint;
    } else {
        list->maximumTodoIndex = qMax(int(list->maximumTodoIndex), index);

        endpoint->next = list->todo;
        if (endpoint->next) endpoint->next->prev = &endpoint->next;
        endpoint->prev = &list->todo;
        list->todo = endpoint;
    }
}

void QQmlData::disconnectNotifiers(QQmlData::DeleteNotifyList doDelete)
{
    // Can only happen on "home" thread. We apply relaxed semantics when loading  the atomics.
    if (NotifyList *list = notifyList.loadRelaxed()) {
        while (QQmlNotifierEndpoint *todo = list->todo)
            todo->disconnect();
        for (int ii = 0; ii < list->notifiesSize; ++ii) {
            while (QQmlNotifierEndpoint *ep = list->notifies[ii])
                ep->disconnect();
        }
        free(list->notifies);

        if (doDelete == DeleteNotifyList::Yes) {
            // We can only get here from QQmlData::destroyed(), and that can only come from the
            // the QObject dtor. If you're still sending signals at that point you have UB already
            // without any threads. Therefore, it's enough to apply relaxed semantics.
            notifyList.storeRelaxed(nullptr);
            delete list;
        } else {
            // We can use relaxed semantics here. The worst thing that can happen is that some
            // signal is falsely reported as connected. Signal connectedness across threads
            // is not quite deterministic anyway.
            list->connectionMask.storeRelaxed(0);
            list->maximumTodoIndex = 0;
            list->notifiesSize = 0;
            list->notifies = nullptr;

        }
    }
}

QHash<QQmlAttachedPropertiesFunc, QObject *> *QQmlData::attachedProperties() const
{
    if (!extendedData) extendedData = new QQmlDataExtended;
    return &extendedData->attachedProperties;
}

void QQmlData::removeFromContext()
{
    if (nextContextObject)
        nextContextObject->prevContextObject = prevContextObject;
    if (prevContextObject)
        *prevContextObject = nextContextObject;
    else if (outerContext && outerContext->ownedObjects() == this)
        outerContext->setOwnedObjects(nextContextObject);

    nextContextObject = nullptr;
    prevContextObject = nullptr;
    outerContext = nullptr;
    context = nullptr;
}

void QQmlData::clearBindings()
{
    if (QQmlAbstractBinding *binding = std::exchange(bindings, nullptr)) {
        for (QQmlAbstractBinding *next = binding; next; next = next->nextBinding())
            next->setAddedToObject(false);
        if (!binding->ref.deref())
            delete binding;
    }
}

bool QQmlData::clearSignalHandlers()
{
    for (QQmlBoundSignal *signalHandler = std::exchange(signalHandlers, nullptr); signalHandler;) {
        if (signalHandler->isNotifying()) {
            signalHandlers = signalHandler;
            return false;
        }

        QQmlBoundSignal *next = signalHandler->m_nextSignal;
        signalHandler->m_prevSignal = nullptr;
        signalHandler->m_nextSignal = nullptr;
        delete signalHandler;
        signalHandler = next;
    }

    return true;
}

void QQmlData::clear()
{
    removeFromContext();
    clearBindings();

    compilationUnit.reset();
    qDeleteAll(std::exchange(deferredData, {}));

    if (!clearSignalHandlers())
        qFatal("Can't clear QQmlData from signal handler");

    BindingBitsType *bits = (bindingBitsArraySize == InlineBindingArraySize)
            ? bindingBitsValue
            : bindingBits;
    memset(bits, 0, bindingBitsArraySize * sizeof(BindingBitsType));

    propertyCache.reset();
    ownContext.reset();

    disconnectNotifiers(DeleteNotifyList::No);

    delete std::exchange(extendedData, nullptr);
    propertyObservers.clear();

    lineNumber = 0;
    columnNumber = 0;
    rootObjectInCreation = false;
    hasInterceptorMetaObject = false;
    hasVMEMetaObject = false;
    cuObjectIndex = -1;
}

void QQmlData::destroyed(QObject *object)
{
    removeFromContext();
    clearBindings();

    compilationUnit.reset();
    qDeleteAll(deferredData);
    deferredData.clear();

    if (!clearSignalHandlers()) {
        // The object is being deleted during signal handler evaluation.
        // This will cause a crash due to invalid memory access when the
        // evaluation has completed.
        // Abort with a friendly message instead.
        QString locationString;
        QQmlBoundSignalExpression *expr = signalHandlers->expression();
        if (expr) {
            QQmlSourceLocation location = expr->sourceLocation();
            if (location.sourceFile.isEmpty())
                location.sourceFile = QStringLiteral("<Unknown File>");
            locationString.append(location.sourceFile);
            locationString.append(QStringLiteral(":%0: ").arg(location.line));
            QString source = expr->expression();
            if (source.size() > 100) {
                source.truncate(96);
                source.append(QLatin1String(" ..."));
            }
            locationString.append(source);
        } else {
            locationString = QStringLiteral("<Unknown Location>");
        }
        qFatal("Object %p destroyed while one of its QML signal handlers is in progress.\n"
               "Most likely the object was deleted synchronously (use QObject::deleteLater() "
               "instead), or the application is running a nested event loop.\n"
               "This behavior is NOT supported!\n"
               "%s", object, qPrintable(locationString));
    }

    if (bindingBitsArraySize > InlineBindingArraySize)
        free(bindingBits);

    if (propertyCache)
        propertyCache.reset();

    ownContext.reset();

    while (guards) {
        auto *guard = guards;
        guard->setObject(nullptr);
        if (guard->objectDestroyed)
            guard->objectDestroyed(guard);
    }

    disconnectNotifiers(DeleteNotifyList::Yes);

    if (extendedData)
        delete extendedData;

    // Dispose the handle.
    jsWrapper.clear();

    if (ownMemory)
        delete this;
    else
        this->~QQmlData();
}

QQmlData::BindingBitsType *QQmlData::growBits(QObject *obj, int bit)
{
    BindingBitsType *bits = (bindingBitsArraySize == InlineBindingArraySize) ? bindingBitsValue : bindingBits;
    int props = QQmlMetaObject(obj).propertyCount();
    Q_ASSERT(bit < 2 * props);
    Q_UNUSED(bit); // .. for Q_NO_DEBUG mode when the assert above expands to empty

    uint arraySize = (2 * static_cast<uint>(props) + BitsPerType - 1) / BitsPerType;
    Q_ASSERT(arraySize > 1);
    Q_ASSERT(arraySize <= 0xffff); // max for bindingBitsArraySize

    BindingBitsType *newBits = static_cast<BindingBitsType *>(malloc(arraySize*sizeof(BindingBitsType)));
    memcpy(newBits, bits, bindingBitsArraySize * sizeof(BindingBitsType));
    memset(newBits + bindingBitsArraySize, 0, sizeof(BindingBitsType) * (arraySize - bindingBitsArraySize));

    if (bindingBitsArraySize > InlineBindingArraySize)
        free(bits);
    bindingBits = newBits;
    bits = newBits;
    bindingBitsArraySize = arraySize;
    return bits;
}

QQmlData *QQmlData::createQQmlData(QObjectPrivate *priv)
{
    Q_ASSERT(priv);
    Q_ASSERT(!priv->isDeletingChildren);
    priv->declarativeData = new QQmlData(OwnsMemory);
    return static_cast<QQmlData *>(priv->declarativeData);
}

QQmlPropertyCache::ConstPtr QQmlData::createPropertyCache(QObject *object)
{
    QQmlData *ddata = QQmlData::get(object, /*create*/true);
    ddata->propertyCache = QQmlMetaType::propertyCache(object, QTypeRevision {});
    return ddata->propertyCache;
}

QT_END_NAMESPACE
