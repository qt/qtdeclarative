// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#include "qqmlinplacepreviewhandler.h"
#include "qqmlpreviewservice.h"

#include <QtGui/qguiapplication.h>
#include <QtQml/qqmlcomponent.h>
#include <QtQuick/qquickwindow.h>
#include <QtQuick/qquickitem.h>

#include <private/qqmlmetatype_p.h>
#include <private/qv4compileddata_p.h>
#include <private/qqmlpreviewobjectpatch_p.h>
#include <private/qv4mm_p.h>
#include <private/qqmlcomponent_p.h>
#include <private/qv4resolvedtypereference_p.h>

QT_BEGIN_NAMESPACE

struct ComponentUpdate
{
    std::unique_ptr<QQmlComponent> component;
    QQmlRefPointer<QV4::ExecutableCompilationUnit> oldUnit;
    QQmlRefPointer<QV4::ExecutableCompilationUnit> newUnit;
    QQmlPreview::PatchResult patchResult = QQmlPreview::PatchResult::Failed;
};

struct InplaceUpdate
{
    InplaceUpdate(QQmlInPlacePreviewHandler *handler, QQmlEngine *engine)
        : engine(engine), handler(handler)
    {
    }

    QQmlEngine *engine = nullptr;
    std::vector<ComponentUpdate> pendingComponentUpdates;
    std::vector<ComponentUpdate> processedComponentUpdates;
    QList<QQmlRefPointer<QV4::CompiledData::CompilationUnit>> droppedUnits;

    void emitReloadFailure(const QString &message) { emit handler->hotReloadFailure(message); }

    void emitError(const QString &message) { emit handler->error(message); }

private:
    // Sending a signal is fine from a different thread. Anything else not so much.
    QQmlInPlacePreviewHandler *handler = nullptr;
};

QQmlInPlacePreviewHandler::QQmlInPlacePreviewHandler(QObject *parent) : QQmlPreviewHandler(parent)
{
}

QQmlInPlacePreviewHandler::~QQmlInPlacePreviewHandler() = default;

void QQmlInPlacePreviewHandler::connectToService(QQmlPreviewServiceImpl *service)
{
    QQmlPreviewHandler::connectToService(service);
    connect(this, &QQmlInPlacePreviewHandler::hotReloadFailure, service,
            &QQmlPreviewServiceImpl::forwardHotReloadFailure, Qt::DirectConnection);
    connect(service, &QQmlPreviewServiceImpl::drop, this, [this](const QUrl &url) {
        if (!m_droppedUrls.contains(url))
            m_droppedUrls.push_back(url);
    });
    connect(service, &QQmlPreviewServiceImpl::rerun, this, [this]() {
        emit error(QLatin1String("You cannot rerun if in-place updates are enabled."));
    });
    connect(service, &QQmlPreviewServiceImpl::zoom, this, [this](qreal zoomFactor) {
        QQmlPreviewPosition position;
        zoomWindow(currentWindow(), zoomFactor, &position);
    });
}

QQuickWindow *findCurrentWindow()
{
    QQuickWindow *found = nullptr;
    for (QWindow *window : QGuiApplication::allWindows()) {
        if (QQuickWindow *quickWindow = qobject_cast<QQuickWindow *>(window)) {
            if (std::exchange(found, quickWindow))
                return nullptr; // Multiple windows available. We can't decide
        }
    }

    return found;
}

void findCurrentRootObject(QQmlEngine *engine, const QUrl &url, QQmlPreviewHandler *receiver)
{
    // Schedule this on the engine's thread
    QMetaObject::invokeMethod(engine, [engine, url, receiver]() {
        QV4::ExecutionEngine *v4 = engine->handle();
        const QQmlRefPointer<QV4::ExecutableCompilationUnit> unit = v4->compilationUnitForUrl(url);
        if (!unit)
            return;

        const std::vector<QObject *> objects =
                v4->memoryManager->findObjectsForCompilationUnits({ unit->baseCompilationUnit() });
        QPointer<QObject> found;
        for (QObject *object : objects) {
            QQmlData *ddata = QQmlData::get(object);
            if (ddata && ddata->compilationUnit == unit && ddata->cuObjectIndex == 0) {
                if (std::exchange(found, object))
                    return; // Multiple candidates. We can't decide
            }
        }

        // Schedule reply on the debug server thread
        QMetaObject::invokeMethod(receiver, [receiver, found]() {
            if (QQuickItem *rootItem = qobject_cast<QQuickItem *>(found)) {
                receiver->setCurrentRootItem(rootItem);
                receiver->setCurrentWindow(findCurrentWindow());
                return;
            }

            if (QQuickWindow *window = qobject_cast<QQuickWindow *>(found)) {
                receiver->setCurrentWindow(window);
                receiver->setCurrentRootItem(nullptr);
                return;
            }

            receiver->setCurrentRootItem(nullptr);
            receiver->setCurrentWindow(findCurrentWindow());
        });
    });
}

// Walk all compilation units in the engine and replace resolved type references
// that still point to any of the dropped (old) base CUs with freshly compiled ones.
// TODO: This is dangerous. We are manipulating compilation units exposed to multiple
//       engines on potentially multiple threads.
static void updateResolvedTypeReferences(
        QV4::ExecutionEngine *v4,
        const QList<QQmlRefPointer<QV4::CompiledData::CompilationUnit>> &droppedUnits)
{
    const auto allCUs = v4->compilationUnits();
    for (const auto &ecu : allCUs) {
        const auto &baseCU = ecu->baseCompilationUnit();
        for (auto *typeRef : std::as_const(baseCU->resolvedTypes)) {
            if (typeRef->isSelfReference())
                continue;
            const auto &refCU = typeRef->compilationUnit();
            if (!refCU)
                continue;
            for (const auto &oldCU : droppedUnits) {
                if (refCU == oldCU) {
                    const auto newCU = QQmlMetaType::obtainCompilationUnit(oldCU->finalUrl());
                    if (newCU)
                        typeRef->setCompilationUnit(newCU);
                    break;
                }
            }
        }
    }
}

static void updateInplace(QQmlComponent *component, std::shared_ptr<InplaceUpdate> inplaceUpdate)
{
    ComponentUpdate componentUpdate;
    const auto componentUpdateIt =
            std::find_if(inplaceUpdate->pendingComponentUpdates.begin(),
                         inplaceUpdate->pendingComponentUpdates.end(),
                         [component](const ComponentUpdate &engineUpdate) {
                             return engineUpdate.component.get() == component;
                         });

    if (componentUpdateIt == inplaceUpdate->pendingComponentUpdates.end())
        return;

    componentUpdate = std::move(*componentUpdateIt);
    inplaceUpdate->pendingComponentUpdates.erase(componentUpdateIt);

    switch (component->status()) {
    case QQmlComponent::Null:
    case QQmlComponent::Loading:
        Q_UNREACHABLE_RETURN();
    case QQmlComponent::Ready:
        break;
    case QQmlComponent::Error:
        inplaceUpdate->emitError(component->errorString());
        return;
    }

    QV4::ExecutionEngine *v4 = component->engine()->handle();

    if (const auto oldExecCU = componentUpdate.oldUnit) {
        const auto newExecCU = QQmlComponentPrivate::get(component)->compilationUnit();
        std::vector<QObject *> objects = v4->memoryManager->findObjectsForCompilationUnits(
                { oldExecCU->baseCompilationUnit() });

        const QQmlPreview::PatchResult result =
                QQmlPreview::applyDiff(objects, oldExecCU, newExecCU);
        if (result == QQmlPreview::PatchResult::Failed)
            inplaceUpdate->emitReloadFailure("Could not apply diff");

        componentUpdate.newUnit = newExecCU;
        componentUpdate.patchResult = result;
        inplaceUpdate->processedComponentUpdates.push_back(std::move(componentUpdate));
    }

    // Once all components have been handled, update resolved type references in all
    // compilation units and refresh all bindings
    if (inplaceUpdate->pendingComponentUpdates.empty()) {
        updateResolvedTypeReferences(v4, inplaceUpdate->droppedUnits);
        for (const ComponentUpdate &update : inplaceUpdate->processedComponentUpdates) {
            // Objects patched in place keep their original (still-live) expressions; translate
            // those to the new unit. Rebuilt roots only leave dead expressions behind; null them.
            QQmlPreview::refreshBindings(
                    update.oldUnit,
                    update.patchResult == QQmlPreview::PatchResult::PatchedInPlace ? update.newUnit
                                                                                   : nullptr);
        }
    }
}

void updateEngine(std::shared_ptr<InplaceUpdate> inplaceUpdate, const QList<QUrl> &urls)
{
    QV4::ExecutionEngine *v4 = inplaceUpdate->engine->handle();
    std::vector<QQmlComponent *> components;
    for (const QUrl &url : urls) {
        // First remove any cached instance of the CU
        // NB: Don't remove from the ExecutionEngine's list of CUs. We need those for the GC.
        if (!v4->typeLoader()->removeFromCache(url))
            continue;

        // Hold on to the old unit.
        QQmlRefPointer<QV4::ExecutableCompilationUnit> oldUnit = v4->compilationUnitForUrl(url);

        // Then have it re-compile with updated source code. newUnit and patchResult are filled
        // in by updateInplace() once the component has recompiled and the diff has been applied.
        inplaceUpdate->pendingComponentUpdates.push_back({
                std::make_unique<QQmlComponent>(inplaceUpdate->engine, url),
                std::move(oldUnit),
                {},
                QQmlPreview::PatchResult::Failed,
        });

        // Additionally store in another vector since updateInplace deletes from inplaceUpdate
        components.push_back(inplaceUpdate->pendingComponentUpdates.back().component.get());
    }

    for (QQmlComponent *component : components) {
        switch (component->status()) {
        case QQmlComponent::Null:
        case QQmlComponent::Loading:
            QObject::connect(component, &QQmlComponent::statusChanged, component,
                             [component, inplaceUpdate]() {
                                 switch (component->status()) {
                                 case QQmlComponent::Ready:
                                 case QQmlComponent::Error:
                                     updateInplace(component, inplaceUpdate);
                                     break;
                                 default:
                                     break;
                                 }
                             });
            break;
        case QQmlComponent::Ready:
        case QQmlComponent::Error:
            updateInplace(component, inplaceUpdate);
            break;
        }
    }
}

void QQmlInPlacePreviewHandler::load(const QUrl &url)
{
    // Find the updated objects from the URLs we were asked to drop. Those are the ones that have
    // been updated.

    const QList<QQmlEngine *> seenEngines = engines();
    const QList<QUrl> urls = std::exchange(m_droppedUrls, {});

    QList<QQmlRefPointer<QV4::CompiledData::CompilationUnit>> droppedUnits;
    for (const QUrl &droppedUrl : urls) {
        while (const auto cu = QQmlMetaType::obtainCompilationUnit(droppedUrl)) {
            droppedUnits.append(cu);
            QQmlMetaType::deepClearCompositeType(cu);
        }
    }
    for (QQmlEngine *engine : seenEngines) {
        std::shared_ptr<InplaceUpdate> inplaceUpdate =
                std::make_shared<InplaceUpdate>(this, engine);
        inplaceUpdate->droppedUnits = droppedUnits;

        // Schedule this on the engine's thread. The shared pointer keeps the update alive as
        // long as necessary.
        QMetaObject::invokeMethod(engine,
                                  [inplaceUpdate, urls]() { updateEngine(inplaceUpdate, urls); });
    }

    // Update current window and root item based on passed URL
    // so that we can zoom and report FPS.
    // We can only do that if we have an unambiguous root item or window.

    if (seenEngines.size() == 1) {
        findCurrentRootObject(seenEngines[0], url, this);
    } else {
        setCurrentRootItem(nullptr);
        setCurrentWindow(findCurrentWindow());
        return;
    }
}

QT_END_NAMESPACE

#include "moc_qqmlinplacepreviewhandler.cpp"
