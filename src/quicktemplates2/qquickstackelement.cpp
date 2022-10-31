// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickstackelement_p_p.h"
#include "qquickstackview_p_p.h"

#include <QtQml/qqmlinfo.h>
#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlcomponent.h>
#include <QtQml/qqmlincubator.h>
#include <QtQml/private/qv4qobjectwrapper_p.h>
#include <QtQml/private/qqmlcomponent_p.h>
#include <QtQml/private/qqmlengine_p.h>
#include <QtQml/private/qqmlincubator_p.h>

QT_BEGIN_NAMESPACE

static QQuickStackViewAttached *attachedStackObject(QQuickStackElement *element)
{
    QQuickStackViewAttached *attached = qobject_cast<QQuickStackViewAttached *>(qmlAttachedPropertiesObject<QQuickStackView>(element->item, false));
    if (attached)
        QQuickStackViewAttachedPrivate::get(attached)->element = element;
    return attached;
}

class QQuickStackIncubator : public QQmlIncubator
{
public:
    QQuickStackIncubator(QQuickStackElement *element)
        : QQmlIncubator(Synchronous),
          element(element)
    {
    }

protected:
    void setInitialState(QObject *object) override
    {
        auto privIncubator = QQmlIncubatorPrivate::get(this);
        element->incubate(object, privIncubator->requiredProperties());
    }

private:
    QQuickStackElement *element;
};

QQuickStackElement::QQuickStackElement()
    : QQuickItemViewTransitionableItem(nullptr)
{
}

QQuickStackElement::~QQuickStackElement()
{
    if (item)
        QQuickItemPrivate::get(item)->removeItemChangeListener(this, QQuickItemPrivate::Destroyed);

    if (ownComponent)
        delete component;

    QQuickStackViewAttached *attached = attachedStackObject(this);
    if (item) {
        if (ownItem) {
            item->setParentItem(nullptr);
            item->deleteLater();
            item = nullptr;
        } else {
            setVisible(false);
            if (!widthValid)
                item->resetWidth();
            if (!heightValid)
                item->resetHeight();
            if (item->parentItem() != originalParent) {
                item->setParentItem(originalParent);
            } else {
                if (attached)
                    QQuickStackViewAttachedPrivate::get(attached)->itemParentChanged(item, nullptr);
            }
        }
    }

    if (attached)
        emit attached->removed();
}

QQuickStackElement *QQuickStackElement::fromString(const QString &str, QQuickStackView *view, QString *error)
{
    QUrl url(str);
    if (!url.isValid()) {
        *error = QStringLiteral("invalid url: ") + str;
        return nullptr;
    }

    if (url.isRelative())
        url = qmlContext(view)->resolvedUrl(url);

    QQuickStackElement *element = new QQuickStackElement;
    element->component = new QQmlComponent(qmlEngine(view), url, view);
    element->ownComponent = true;
    return element;
}

QQuickStackElement *QQuickStackElement::fromObject(QObject *object, QQuickStackView *view, QString *error)
{
    Q_UNUSED(view);
    QQmlComponent *component = qobject_cast<QQmlComponent *>(object);
    QQuickItem *item = qobject_cast<QQuickItem *>(object);
    if (!component && !item) {
        *error = QQmlMetaType::prettyTypeName(object) + QStringLiteral(" is not supported. Must be Item or Component.");
        return nullptr;
    }

    QQuickStackElement *element = new QQuickStackElement;
    element->component = qobject_cast<QQmlComponent *>(object);
    element->item = qobject_cast<QQuickItem *>(object);
    if (element->item)
        element->originalParent = element->item->parentItem();
    return element;
}

bool QQuickStackElement::load(QQuickStackView *parent)
{
    setView(parent);
    if (!item) {
        ownItem = true;

        if (component->isLoading()) {
            QObject::connect(component, &QQmlComponent::statusChanged, [this](QQmlComponent::Status status) {
                if (status == QQmlComponent::Ready)
                    load(view);
                else if (status == QQmlComponent::Error)
                    QQuickStackViewPrivate::get(view)->warn(component->errorString().trimmed());
            });
            return true;
        }

        QQmlContext *context = component->creationContext();
        if (!context)
            context = qmlContext(parent);

        QQuickStackIncubator incubator(this);
        component->create(incubator, context);
        if (component->isError())
            QQuickStackViewPrivate::get(parent)->warn(component->errorString().trimmed());
    } else {
        initialize(/*required properties=*/nullptr);
    }
    return item;
}

void QQuickStackElement::incubate(QObject *object, RequiredProperties *requiredProperties)
{
    item = qmlobject_cast<QQuickItem *>(object);
    if (item) {
        QQmlEngine::setObjectOwnership(item, QQmlEngine::CppOwnership);
        item->setParent(view);
        initialize(requiredProperties);
    }
}

void QQuickStackElement::initialize(RequiredProperties *requiredProperties)
{
    if (!item || init)
        return;

    QQuickItemPrivate *p = QQuickItemPrivate::get(item);
    if (!(widthValid = p->widthValid()))
        item->setWidth(view->width());
    if (!(heightValid = p->heightValid()))
        item->setHeight(view->height());
    item->setParentItem(view);

    if (!properties.isUndefined()) {
        QQmlEngine *engine = qmlEngine(view);
        Q_ASSERT(engine);
        QV4::ExecutionEngine *v4 = QQmlEnginePrivate::getV4Engine(engine);
        Q_ASSERT(v4);
        QV4::Scope scope(v4);
        QV4::ScopedValue ipv(scope, properties.value());
        QV4::Scoped<QV4::QmlContext> qmlContext(scope, qmlCallingContext.value());
        QV4::ScopedValue qmlObject(scope, QV4::QObjectWrapper::wrap(v4, item));
        QQmlComponentPrivate::setInitialProperties(v4, qmlContext, qmlObject, ipv, requiredProperties, item);
        properties.clear();
    }

    if (requiredProperties && !requiredProperties->empty()) {
        QString error;
        for (const auto &property: *requiredProperties) {
            error += QLatin1String("Property %1 was marked as required but not set.\n")
                    .arg(property.propertyName);
        }
        QQuickStackViewPrivate::get(view)->warn(error);
        item = nullptr;
    } else {
        p->addItemChangeListener(this, QQuickItemPrivate::Destroyed);
    }

    init = true;
}

void QQuickStackElement::setIndex(int value)
{
    if (index == value)
        return;

    index = value;
    QQuickStackViewAttached *attached = attachedStackObject(this);
    if (attached)
        emit attached->indexChanged();
}

void QQuickStackElement::setView(QQuickStackView *value)
{
    if (view == value)
        return;

    view = value;
    QQuickStackViewAttached *attached = attachedStackObject(this);
    if (attached)
        emit attached->viewChanged();
}

void QQuickStackElement::setStatus(QQuickStackView::Status value)
{
    if (status == value)
        return;

    status = value;
    QQuickStackViewAttached *attached = attachedStackObject(this);
    if (!attached)
        return;

    switch (value) {
    case QQuickStackView::Inactive:
        emit attached->deactivated();
        break;
    case QQuickStackView::Deactivating:
        emit attached->deactivating();
        break;
    case QQuickStackView::Activating:
        emit attached->activating();
        break;
    case QQuickStackView::Active:
        emit attached->activated();
        break;
    default:
        Q_UNREACHABLE();
        break;
    }

    emit attached->statusChanged();
}

void QQuickStackElement::setVisible(bool visible)
{
    QQuickStackViewAttached *attached = attachedStackObject(this);
    if (!item || (attached && QQuickStackViewAttachedPrivate::get(attached)->explicitVisible))
        return;

    item->setVisible(visible);
}

void QQuickStackElement::transitionNextReposition(QQuickItemViewTransitioner *transitioner, QQuickItemViewTransitioner::TransitionType type, bool asTarget)
{
    if (transitioner)
        transitioner->transitionNextReposition(this, type, asTarget);
}

bool QQuickStackElement::prepareTransition(QQuickItemViewTransitioner *transitioner, const QRectF &viewBounds)
{
    if (transitioner) {
        if (item) {
            QQuickAnchors *anchors = QQuickItemPrivate::get(item)->_anchors;
            // TODO: expose QQuickAnchorLine so we can test for other conflicting anchors
            if (anchors && (anchors->fill() || anchors->centerIn()))
                qmlWarning(item) << "StackView has detected conflicting anchors. Transitions may not execute properly.";
        }

        // TODO: add force argument to QQuickItemViewTransitionableItem::prepareTransition()?
        nextTransitionToSet = true;
        nextTransitionFromSet = true;
        nextTransitionFrom += QPointF(1, 1);
        return QQuickItemViewTransitionableItem::prepareTransition(transitioner, index, viewBounds);
    }
    return false;
}

void QQuickStackElement::startTransition(QQuickItemViewTransitioner *transitioner, QQuickStackView::Status status)
{
    setStatus(status);
    if (transitioner)
        QQuickItemViewTransitionableItem::startTransition(transitioner, index);
}

void QQuickStackElement::completeTransition(QQuickTransition *quickTransition)
{
    QQuickItemViewTransitionableItem::completeTransition(quickTransition);
}

void QQuickStackElement::itemDestroyed(QQuickItem *)
{
    item = nullptr;
}

QT_END_NAMESPACE
