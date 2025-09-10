// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickdeferredexecute_p_p.h"

#include <QtCore/qhash.h>
#include <QtQml/qqmlengine.h>
#include <QtQml/private/qqmldata_p.h>
#include <QtQml/private/qqmlcomponent_p.h>
#include <QtQml/private/qqmlobjectcreator_p.h>

#include <private/qv4resolvedtypereference_p.h>

#include <deque>

QT_BEGIN_NAMESPACE

/*!
    \internal

    For the history behind why these functions were introduced, see
    the comments of QTBUG-50992, specifically
    \l {https://bugreports.qt.io/browse/QTBUG-50992?focusedId=325677&page=com.atlassian.jira.plugin.system.issuetabpanels:comment-tabpanel#comment-325677}{this}.

    The first commit to the QML engine to support deferred properties seems to
    be 59b8d719d6122d86a4cc650911788cc4d778ce29.

    The first commit to add support for it in Controls was
    458eb65f730178bc93ba7b18f0e4dd2a13efad2e.

    In short, deferred execution solved two issues:

    \list 1
    \li Incubation issues when using asynchronous loaders, AKA QTBUG-50992.
    \li Performance issues from creating two items unnecessarily when a
        styled control was customized, which is explained in more detail
        in the commit message of 458eb65f730178bc93ba7b18f0e4dd2a13efad2e.
    \endlist

    \sa qmlExecuteDeferred
*/

namespace QtQuickPrivate {

static void cancelDeferred(QQmlData *ddata, int propertyIndex)
{
    auto dit = ddata->deferredData.rbegin();
    while (dit != ddata->deferredData.rend()) {
        (*dit)->bindings.remove(propertyIndex);
        ++dit;
    }
}

static bool beginDeferred(QQmlEnginePrivate *enginePriv, const QQmlProperty &property, QQmlComponentPrivate::DeferredState *deferredState)
{
    QObject *object = property.object();
    QQmlData *ddata = QQmlData::get(object);
    Q_ASSERT(!ddata->deferredData.isEmpty());

    if (!ddata->propertyCache)
        ddata->propertyCache = QQmlMetaType::propertyCache(object->metaObject());

    int propertyIndex = property.index();
    int wasInProgress = enginePriv->inProgressCreations;

    /* we don't want deferred properties to suddenly depend on arbitrary
       other properties which might have trigerred the construction of
       objects as a consequence of a read.
     */
    auto bindingStatus = QtPrivate::suspendCurrentBindingStatus();
    auto cleanup = qScopeGuard([&](){
        QtPrivate::restoreBindingStatus(bindingStatus);
    });

    for (auto dit = ddata->deferredData.rbegin(); dit != ddata->deferredData.rend(); ++dit) {
        QQmlData::DeferredData *deferData = *dit;

        auto bindings = deferData->bindings;
        auto range = bindings.equal_range(propertyIndex);
        if (range.first == bindings.end())
            continue;

        QQmlComponentPrivate::ConstructionState state;
        state.setCompletePending(true);

        QQmlContextData *creationContext = nullptr;

        state.initCreator(deferData->context->parent(), deferData->compilationUnit, creationContext, deferData->inlineComponentName);

        enginePriv->inProgressCreations++;

        std::deque<const QV4::CompiledData::Binding *> reversedBindings;
        std::copy(range.first, range.second, std::front_inserter(reversedBindings));
        state.creator()->beginPopulateDeferred(deferData->context);
        for (const QV4::CompiledData::Binding *binding : reversedBindings)
            state.creator()->populateDeferredBinding(property, deferData->deferredIdx, binding);
        state.creator()->finalizePopulateDeferred();
        state.appendCreatorErrors();

        deferredState->push_back(std::move(state));

        // Cleanup any remaining deferred bindings for this property, also in inner contexts,
        // to avoid executing them later and overriding the property that was just populated.
        cancelDeferred(ddata, propertyIndex);
        break;
    }

    return enginePriv->inProgressCreations > wasInProgress;
}

void beginDeferred(QObject *object, const QString &property,
                   QQuickUntypedDeferredPointer *delegate, bool isOwnState)
{
    QQmlData *data = QQmlData::get(object);
    if (data && !data->deferredData.isEmpty() && !data->wasDeleted(object) && data->context) {
        QQmlEnginePrivate *ep = QQmlEnginePrivate::get(data->context->engine());

        QQmlComponentPrivate::DeferredState state;
        if (beginDeferred(ep, QQmlProperty(object, property), &state)) {
            if (QQmlComponentPrivate::DeferredState *delegateState = delegate->deferredState())
                delegateState->swap(state);
        } else if (isOwnState) {
            delegate->clearDeferredState();
        }

        // Release deferred data for those compilation units that no longer have deferred bindings
        data->releaseDeferredData();
    } else if (isOwnState) {
        delegate->clearDeferredState();
    }
}

void cancelDeferred(QObject *object, const QString &property)
{
    QQmlData *data = QQmlData::get(object);
    if (data)
        cancelDeferred(data, QQmlProperty(object, property).index());
}

void completeDeferred(QObject *object, const QString &property, QQuickUntypedDeferredPointer *delegate)
{
    Q_UNUSED(property);
    QQmlComponentPrivate::DeferredState *state = delegate->deferredState();
    if (!state)
        return;

    QQmlData *data = QQmlData::get(object);
    if (data && !data->wasDeleted(object)) {
        /* we don't want deferred properties to suddenly depend on arbitrary
           other properties which might have trigerred the construction of
           objects as a consequence of a read.
         */
        auto bindingStatus = QtPrivate::suspendCurrentBindingStatus();
        auto cleanup = qScopeGuard([&](){
            QtPrivate::restoreBindingStatus(bindingStatus);
        });

        QQmlComponentPrivate::DeferredState localState = std::move(*state);
        delegate->clearDeferredState();
        QQmlEnginePrivate *ep = QQmlEnginePrivate::get(data->context->engine());
        QQmlComponentPrivate::completeDeferred(ep, &localState);
    } else {
        delegate->clearDeferredState();
    }
}

} // QtQuickPrivate

QT_END_NAMESPACE
