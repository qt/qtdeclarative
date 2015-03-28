/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Quick Controls module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL3$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or later as published by the Free
** Software Foundation and appearing in the file LICENSE.GPL included in
** the packaging of this file. Please review the following information to
** ensure the GNU General Public License version 2.0 requirements will be
** met: http://www.gnu.org/licenses/gpl-2.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qquickabstractstackview_p_p.h"

#include <QtQml/qqmllist.h>
#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlcomponent.h>
#include <QtQml/qqmlincubator.h>
#include <QtQml/private/qqmlcomponent_p.h>
#include <QtQuick/private/qquickanimation_p.h>
#include <QtQuick/private/qquicktransition_p.h>
#include <QtQuick/private/qquickitemviewtransition_p.h>

QT_BEGIN_NAMESPACE

static QQuickStackAttached *attachedStackObject(QQuickItem *item)
{
    return qobject_cast<QQuickStackAttached *>(qmlAttachedPropertiesObject<QQuickAbstractStackView>(item, false));
}

class QQuickStackIncubator : public QQmlIncubator
{
public:
    QQuickStackIncubator(QQuickStackElement *element) : QQmlIncubator(Synchronous), element(element) { }

protected:
    void setInitialState(QObject *object) Q_DECL_OVERRIDE { element->initItem(object); }

private:
    QQuickStackElement *element;
};

QQuickStackElement::QQuickStackElement() : QQuickItemViewTransitionableItem(Q_NULLPTR),
    index(-1), removal(false), ownItem(false), ownComponent(false),
    context(Q_NULLPTR), component(Q_NULLPTR), incubator(Q_NULLPTR), view(Q_NULLPTR),
    status(QQuickAbstractStackView::Inactive)
{
}

QQuickStackElement::~QQuickStackElement()
{
    if (item) {
        QQuickItemPrivate::get(item)->removeItemChangeListener(this, QQuickItemPrivate::Destroyed);

        QQuickStackAttached *attached = attachedStackObject(item);
        if (attached)
            QQuickStackAttachedPrivate::get(attached)->reset();
    }

    if (ownComponent)
        delete component;

    if (ownItem && item) {
        item->setParentItem(Q_NULLPTR);
        item->deleteLater();
        item = Q_NULLPTR;
    } else if (item) {
        item->setVisible(false);
        item->setParentItem(originalParent);
    }

    delete context;
    delete incubator;
}

QQuickStackElement *QQuickStackElement::fromString(const QString &str, QQuickAbstractStackView *view)
{
    QQuickStackElement *element = new QQuickStackElement;
    element->component = new QQmlComponent(qmlEngine(view), QUrl(str), view);
    element->ownComponent = true;
    return element;
}

QQuickStackElement *QQuickStackElement::fromObject(QObject *object, QQuickAbstractStackView *view)
{
    QQuickStackElement *element = new QQuickStackElement;
    element->component = qobject_cast<QQmlComponent *>(object);
    if (!element->component) {
        element->component = new QQmlComponent(qmlEngine(view), view);
        element->ownComponent = true;
    }
    element->item = qobject_cast<QQuickItem *>(object);
    if (element->item) {
        element->originalParent = element->item->parentItem();
        element->item->setParentItem(view);
        QQuickItemPrivate::get(element->item)->addItemChangeListener(element, QQuickItemPrivate::Destroyed);
    }
    return element;
}

bool QQuickStackElement::load(QQuickAbstractStackView *parent)
{
    view = parent;
    if (!item) {
        ownItem = true;

        QQmlContext *creationContext = component->creationContext();
        if (!creationContext)
            creationContext = qmlContext(parent);
        context = new QQmlContext(creationContext);
        context->setContextObject(parent);

        delete incubator;
        incubator = new QQuickStackIncubator(this);
        component->create(*incubator, context);
    }
    initProperties();
    return item;
}

void QQuickStackElement::initItem(QObject *object)
{
    item = qmlobject_cast<QQuickItem *>(object);
    if (item) {
        QQmlEngine::setObjectOwnership(item, QQmlEngine::CppOwnership);
        QQuickItemPrivate *p = QQuickItemPrivate::get(item);
        if (!p->widthValid) {
            item->setWidth(view->width());
            p->widthValid = false;
        }
        if (!p->heightValid) {
            item->setHeight(view->height());
            p->heightValid = false;
        }
        item->setParentItem(view);
        p->addItemChangeListener(this, QQuickItemPrivate::Destroyed);
    }
    initProperties();
}

void QQuickStackElement::initProperties()
{
    if (!properties.isUndefined()) {
        QQmlComponentPrivate *d = QQmlComponentPrivate::get(component);
        Q_ASSERT(d && d->engine);
        QV4::ExecutionEngine *v4 = qmlGlobal.engine();
        Q_ASSERT(v4);
        QV4::Scope scope(v4);
        QV4::ScopedValue ipv(scope, properties.value());
        d->initializeObjectWithInitialProperties(*qmlGlobal.valueRef(), ipv, item);
        properties.clear();
    }
}

void QQuickStackElement::setIndex(int value)
{
    if (index != value) {
        index = value;
        QQuickStackAttached *attached = attachedStackObject(item);
        if (attached)
            emit attached->indexChanged();
    }
}

void QQuickStackElement::setStatus(QQuickAbstractStackView::Status value)
{
    if (status != value) {
        status = value;
        QQuickStackAttached *attached = attachedStackObject(item);
        if (attached)
            emit attached->statusChanged();
    }
}

void QQuickStackElement::transitionNextReposition(QQuickItemViewTransitioner *transitioner, QQuickItemViewTransitioner::TransitionType type, bool asTarget)
{
    if (transitioner)
        transitioner->transitionNextReposition(this, type, asTarget);
}

bool QQuickStackElement::prepareTransition(QQuickItemViewTransitioner *transitioner, const QRectF &viewBounds)
{
    if (transitioner) {
        // TODO: add force argument to QQuickItemViewTransitionableItem::prepareTransition()?
        nextTransitionToSet = true;
        nextTransitionFromSet = true;
        nextTransitionFrom += QPointF(1, 1);
        return QQuickItemViewTransitionableItem::prepareTransition(transitioner, index, viewBounds);
    }
    return false;
}

void QQuickStackElement::startTransition(QQuickItemViewTransitioner *transitioner)
{
    if (transitioner)
        QQuickItemViewTransitionableItem::startTransition(transitioner, index);
}

void QQuickStackElement::itemDestroyed(QQuickItem *)
{
    item = Q_NULLPTR;
}

QQuickAbstractStackViewPrivate::QQuickAbstractStackViewPrivate() : currentItem(Q_NULLPTR), transitioner(Q_NULLPTR)
{
}

void QQuickAbstractStackViewPrivate::setCurrentItem(QQuickItem *item)
{
    Q_Q(QQuickAbstractStackView);
    if (currentItem != item) {
        currentItem = item;
        if (item)
            item->setVisible(true);
        emit q->currentItemChanged();
    }
}

static bool initProperties(QQuickStackElement *element, const QV4::Value &props, QQmlV4Function *args)
{
    if (props.isObject()) {
        const QV4::QObjectWrapper *wrapper = props.as<QV4::QObjectWrapper>();
        if (!wrapper) {
            QV4::ExecutionEngine *v4 = args->v4engine();
            element->properties.set(v4, props);
            element->qmlGlobal.set(v4, args->qmlGlobal());
            return true;
        }
    }
    return false;
}

QList<QQuickStackElement *> QQuickAbstractStackViewPrivate::parseElements(QQmlV4Function *args, int from)
{
    QV4::ExecutionEngine *v4 = args->v4engine();
    QV4::Scope scope(v4);

    QList<QQuickStackElement *> elements;

    int argc = args->length();
    for (int i = from; i < argc; ++i) {
        QV4::ScopedValue arg(scope, (*args)[i]);
        if (QV4::ArrayObject *array = arg->asArrayObject()) {
            int len = array->getLength();
            for (int j = 0; j < len; ++j) {
                QV4::ScopedValue value(scope, array->getIndexed(j));
                QQuickStackElement *element = createElement(value);
                if (element) {
                    if (j < len - 1) {
                        QV4::ScopedValue props(scope, array->getIndexed(j + 1));
                        if (initProperties(element, props, args))
                            ++j;
                    }
                    elements += element;
                }
            }
        } else {
            QQuickStackElement *element = createElement(arg);
            if (element) {
                if (i < argc - 1) {
                    QV4::ScopedValue props(scope, (*args)[i + 1]);
                    if (initProperties(element, props, args))
                        ++i;
                }
                elements += element;
            }
        }
    }
    return elements;
}

QQuickStackElement *QQuickAbstractStackViewPrivate::findElement(QQuickItem *item) const
{
    if (item) {
        foreach (QQuickStackElement *e, elements) {
            if (e->item == item)
                return e;
        }
    }
    return Q_NULLPTR;
}

QQuickStackElement *QQuickAbstractStackViewPrivate::findElement(const QV4::Value &value) const
{
    if (const QV4::QObjectWrapper *o = value.as<QV4::QObjectWrapper>())
        return findElement(qobject_cast<QQuickItem *>(o->object()));
    return Q_NULLPTR;
}

QQuickStackElement *QQuickAbstractStackViewPrivate::createElement(const QV4::Value &value)
{
    Q_Q(QQuickAbstractStackView);
    if (QV4::String *s = value.asString())
        return QQuickStackElement::fromString(s->toQString(), q);
    if (const QV4::QObjectWrapper *o = value.as<QV4::QObjectWrapper>())
        return QQuickStackElement::fromObject(o->object(), q);
    return Q_NULLPTR;
}

bool QQuickAbstractStackViewPrivate::pushElements(const QList<QQuickStackElement *> &elems)
{
    Q_Q(QQuickAbstractStackView);
    if (!elems.isEmpty()) {
        foreach (QQuickStackElement *e, elems) {
            e->setIndex(elements.count());
            elements += e;
        }
        return elements.top()->load(q);
    }
    return false;
}

bool QQuickAbstractStackViewPrivate::pushElement(QQuickStackElement *element)
{
    if (element)
        return pushElements(QList<QQuickStackElement *>() << element);
    return false;
}

bool QQuickAbstractStackViewPrivate::popElements(QQuickStackElement *element)
{
    Q_Q(QQuickAbstractStackView);
    if (elements.count() > 1) {
        while (elements.count() > 1 && elements.top() != element) {
            delete elements.pop();
            if (!element)
                break;
        }
    }
    return elements.top()->load(q);
}

bool QQuickAbstractStackViewPrivate::replaceElements(QQuickStackElement *target, const QList<QQuickStackElement *> &elems)
{
    if (target) {
        while (!elements.isEmpty()) {
            QQuickStackElement* top = elements.pop();
            delete top;
            if (top == target)
                break;
        }
    }
    return pushElements(elems);
}

void QQuickAbstractStackViewPrivate::ensureTransitioner()
{
    if (!transitioner) {
        transitioner = new QQuickItemViewTransitioner;
        transitioner->setChangeListener(this);
    }
}

void QQuickAbstractStackViewPrivate::popTransition(QQuickStackElement *enter, QQuickStackElement *exit, const QRectF &viewBounds, bool immediate)
{
    Q_Q(QQuickAbstractStackView);
    if (exit) {
        exit->setStatus(QQuickAbstractStackView::Deactivating);
        exit->transitionNextReposition(transitioner, QQuickItemViewTransitioner::RemoveTransition, true);
    }
    if (enter) {
        enter->setStatus(QQuickAbstractStackView::Activating);
        enter->transitionNextReposition(transitioner, QQuickItemViewTransitioner::RemoveTransition, false);
    }

    if (exit && exit->prepareTransition(transitioner, viewBounds)) {
        if (immediate)
            completeTransition(exit, transitioner->removeTransition);
        else
            exit->startTransition(transitioner);
    }
    if (enter && enter->prepareTransition(transitioner, viewBounds)) {
        if (immediate)
            completeTransition(enter, transitioner->removeDisplacedTransition);
        else
            enter->startTransition(transitioner);
    }

    if (!immediate)
        emit q->busyChanged();

    if (transitioner)
        transitioner->resetTargetLists();
}

void QQuickAbstractStackViewPrivate::pushTransition(QQuickStackElement *enter, QQuickStackElement *exit, const QRectF &viewBounds, bool immediate)
{
    Q_Q(QQuickAbstractStackView);
    if (enter) {
        enter->setStatus(QQuickAbstractStackView::Activating);
        enter->transitionNextReposition(transitioner, QQuickItemViewTransitioner::AddTransition, true);
    }
    if (exit) {
        exit->setStatus(QQuickAbstractStackView::Deactivating);
        exit->transitionNextReposition(transitioner, QQuickItemViewTransitioner::AddTransition, false);
    }

    if (enter && enter->prepareTransition(transitioner, viewBounds)) {
        if (immediate)
            completeTransition(enter, transitioner->addTransition);
        else
            enter->startTransition(transitioner);
    }
    if (exit && exit->prepareTransition(transitioner, viewBounds)) {
        if (immediate)
            completeTransition(exit, transitioner->addDisplacedTransition);
        else
            exit->startTransition(transitioner);
    }

    if (!immediate)
        emit q->busyChanged();

    if (transitioner)
        transitioner->resetTargetLists();
}

void QQuickAbstractStackViewPrivate::replaceTransition(QQuickStackElement *enter, QQuickStackElement *exit, const QRectF &viewBounds, bool immediate)
{
    Q_Q(QQuickAbstractStackView);
    if (enter) {
        enter->setStatus(QQuickAbstractStackView::Activating);
        enter->transitionNextReposition(transitioner, QQuickItemViewTransitioner::AddTransition, true);
    }
    if (exit) {
        exit->removal = true;
        exit->setStatus(QQuickAbstractStackView::Deactivating);
        exit->transitionNextReposition(transitioner, QQuickItemViewTransitioner::AddTransition, false);
    }

    if (enter && enter->prepareTransition(transitioner, viewBounds)) {
        if (immediate)
            completeTransition(enter, transitioner->addTransition);
        else
            enter->startTransition(transitioner);
    }
    if (exit && exit->prepareTransition(transitioner, viewBounds)) {
        if (immediate)
            completeTransition(exit, transitioner->addDisplacedTransition);
        else
            exit->startTransition(transitioner);
    }

    if (!immediate)
        emit q->busyChanged();

    if (transitioner)
        transitioner->resetTargetLists();
}

void QQuickAbstractStackViewPrivate::completeTransition(QQuickStackElement *element, QQuickTransition *transition)
{
    if (transition) {
        // TODO: add a proper way to complete a transition
        QQmlListProperty<QQuickAbstractAnimation> animations = transition->animations();
        int count = animations.count(&animations);
        for (int i = 0; i < count; ++i) {
            QQuickAbstractAnimation *anim = animations.at(&animations, i);
            anim->complete();
        }
        viewItemTransitionFinished(element);
    }
}

void QQuickAbstractStackViewPrivate::viewItemTransitionFinished(QQuickItemViewTransitionableItem *transitionable)
{
    Q_Q(QQuickAbstractStackView);

    QQuickStackElement *element = static_cast<QQuickStackElement *>(transitionable);
    if (element->status == QQuickAbstractStackView::Activating) {
        element->setStatus(QQuickAbstractStackView::Active);
    } else if (element->status == QQuickAbstractStackView::Deactivating) {
        element->setStatus(QQuickAbstractStackView::Inactive);
        element->item->setVisible(false);
        if (element->removal || element->isPendingRemoval())
            delete element;
    }

    if (transitioner->runningJobs.isEmpty())
        emit q->busyChanged();
}

QT_END_NAMESPACE
