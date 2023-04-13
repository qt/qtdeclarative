// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickstackview_p_p.h"
#include "qquickstackelement_p_p.h"
#if QT_CONFIG(quick_viewtransitions)
#include "qquickstacktransition_p_p.h"
#endif

#include <QtQml/qqmlinfo.h>
#include <QtQml/qqmllist.h>
#include <QtQml/private/qv4qmlcontext_p.h>
#include <QtQml/private/qv4qobjectwrapper_p.h>
#include <QtQml/private/qv4variantobject_p.h>
#include <QtQml/private/qv4urlobject_p.h>
#include <QtQuick/private/qquickanimation_p.h>
#include <QtQuick/private/qquicktransition_p.h>

QT_BEGIN_NAMESPACE

void QQuickStackViewPrivate::warn(const QString &error)
{
    Q_Q(QQuickStackView);
    if (operation.isEmpty())
        qmlWarning(q) << error;
    else
        qmlWarning(q) << operation << ": " << error;
}

void QQuickStackViewPrivate::warnOfInterruption(const QString &attemptedOperation)
{
    Q_Q(QQuickStackView);
    qmlWarning(q) << "cannot " << attemptedOperation << " while already in the process of completing a " << operation;
}

void QQuickStackViewPrivate::setCurrentItem(QQuickStackElement *element)
{
    Q_Q(QQuickStackView);
    QQuickItem *item = element ? element->item : nullptr;
    if (currentItem == item)
        return;

    currentItem = item;
    if (element)
        element->setVisible(true);
    if (item)
        item->setFocus(true);
    emit q->currentItemChanged();
}

static bool initProperties(QQuickStackElement *element, const QV4::Value &props, QQmlV4Function *args)
{
    if (props.isObject()) {
        const QV4::QObjectWrapper *wrapper = props.as<QV4::QObjectWrapper>();
        if (!wrapper) {
            QV4::ExecutionEngine *v4 = args->v4engine();
            element->properties.set(v4, props);
            element->qmlCallingContext.set(v4, v4->qmlContext());
            return true;
        }
    }
    return false;
}

QList<QQuickStackElement *> QQuickStackViewPrivate::parseElements(int from, QQmlV4Function *args, QStringList *errors)
{
    QV4::ExecutionEngine *v4 = args->v4engine();
    auto context = v4->callingQmlContext();
    QV4::Scope scope(v4);

    QList<QQuickStackElement *> elements;

    int argc = args->length();
    for (int i = from; i < argc; ++i) {
        QV4::ScopedValue arg(scope, (*args)[i]);
        if (QV4::ArrayObject *array = arg->as<QV4::ArrayObject>()) {
            const uint len = uint(array->getLength());
            for (uint j = 0; j < len; ++j) {
                QString error;
                QV4::ScopedValue value(scope, array->get(j));
                QQuickStackElement *element = createElement(value, context, &error);
                if (element) {
                    if (j < len - 1) {
                        QV4::ScopedValue props(scope, array->get(j + 1));
                        if (initProperties(element, props, args))
                            ++j;
                    }
                    elements += element;
                } else if (!error.isEmpty()) {
                    *errors += error;
                }
            }
        } else {
            QString error;
            QQuickStackElement *element = createElement(arg, context, &error);
            if (element) {
                if (i < argc - 1) {
                    QV4::ScopedValue props(scope, (*args)[i + 1]);
                    if (initProperties(element, props, args))
                        ++i;
                }
                elements += element;
            } else if (!error.isEmpty()) {
                *errors += error;
            }
        }
    }
    return elements;
}

QQuickStackElement *QQuickStackViewPrivate::findElement(QQuickItem *item) const
{
    if (item) {
        for (QQuickStackElement *e : std::as_const(elements)) {
            if (e->item == item)
                return e;
        }
    }
    return nullptr;
}

QQuickStackElement *QQuickStackViewPrivate::findElement(const QV4::Value &value) const
{
    if (const QV4::QObjectWrapper *o = value.as<QV4::QObjectWrapper>())
        return findElement(qobject_cast<QQuickItem *>(o->object()));
    return nullptr;
}

static QUrl resolvedUrl(const QUrl &url, const QQmlRefPointer<QQmlContextData> &context)
{
    if (url.isRelative())
        return context->resolvedUrl(url).toString();
    return url;
}

static QString resolvedUrl(const QString &str, const QQmlRefPointer<QQmlContextData> &context)
{
    QUrl url(str);
    if (url.isRelative())
        return context->resolvedUrl(url).toString();
    return str;
}

QQuickStackElement *QQuickStackViewPrivate::createElement(const QV4::Value &value, const QQmlRefPointer<QQmlContextData> &context, QString *error)
{
    Q_Q(QQuickStackView);
    if (const QV4::String *s = value.as<QV4::String>())
        return QQuickStackElement::fromString(resolvedUrl(s->toQString(), context), q, error);
    if (const QV4::QObjectWrapper *o = value.as<QV4::QObjectWrapper>())
        return QQuickStackElement::fromObject(o->object(), q, error);
    if (const QV4::UrlObject *u = value.as<QV4::UrlObject>())
        return QQuickStackElement::fromString(resolvedUrl(u->href(), context), q, error);

    if (value.as<QV4::Object>()) {
        const QVariant data = QV4::ExecutionEngine::toVariant(value, QMetaType::fromType<QUrl>());
        if (data.typeId() == QMetaType::QUrl) {
            return QQuickStackElement::fromString(resolvedUrl(data.toUrl(), context).toString(), q,
                                                  error);
        }
    }

    return nullptr;
}

bool QQuickStackViewPrivate::pushElements(const QList<QQuickStackElement *> &elems)
{
    Q_Q(QQuickStackView);
    if (!elems.isEmpty()) {
        for (QQuickStackElement *e : elems) {
            e->setIndex(elements.size());
            elements += e;
        }
        return elements.top()->load(q);
    }
    return false;
}

bool QQuickStackViewPrivate::pushElement(QQuickStackElement *element)
{
    if (element)
        return pushElements(QList<QQuickStackElement *>() << element);
    return false;
}

bool QQuickStackViewPrivate::popElements(QQuickStackElement *element)
{
    Q_Q(QQuickStackView);
    while (elements.size() > 1 && elements.top() != element) {
        delete elements.pop();
        if (!element)
            break;
    }
    return elements.top()->load(q);
}

bool QQuickStackViewPrivate::replaceElements(QQuickStackElement *target, const QList<QQuickStackElement *> &elems)
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

#if QT_CONFIG(quick_viewtransitions)
void QQuickStackViewPrivate::ensureTransitioner()
{
    if (!transitioner) {
        transitioner = new QQuickItemViewTransitioner;
        transitioner->setChangeListener(this);
    }
}

void QQuickStackViewPrivate::startTransition(const QQuickStackTransition &first, const QQuickStackTransition &second, bool immediate)
{
    if (first.element)
        first.element->transitionNextReposition(transitioner, first.type, first.target);
    if (second.element)
        second.element->transitionNextReposition(transitioner, second.type, second.target);

    if (first.element) {
        // Let the check for immediate happen after prepareTransition() is
        // called, because we need the prepared transition in both branches.
        // Same for the second element.
        if (!first.element->item || !first.element->prepareTransition(transitioner, first.viewBounds) || immediate)
            completeTransition(first.element, first.transition, first.status);
        else
            first.element->startTransition(transitioner, first.status);
    }
    if (second.element) {
        if (!second.element->item || !second.element->prepareTransition(transitioner, second.viewBounds) || immediate)
            completeTransition(second.element, second.transition, second.status);
        else
            second.element->startTransition(transitioner, second.status);
    }

    if (transitioner) {
        setBusy(!transitioner->runningJobs.isEmpty());
        transitioner->resetTargetLists();
    }
}

void QQuickStackViewPrivate::completeTransition(QQuickStackElement *element, QQuickTransition *transition, QQuickStackView::Status status)
{
    element->setStatus(status);
    if (transition) {
        if (element->prepared) {
            // Here we force reading all the animations, even if the desired
            // transition type is StackView.Immediate. After that we force
            // all the animations to complete immediately, without waiting for
            // the animation timer.
            // This allows us to correctly restore all the properties affected
            // by the push/pop animations.
            ACTION_IF_DELETED(element, element->completeTransition(transition), return);
        } else if (element->item) {
            // At least try to move the item to its desired place. This,
            // however, is only a partly correct solution, because a lot more
            // properties can be affected by the transition
            element->item->setPosition(element->nextTransitionTo);
        }
    }
    viewItemTransitionFinished(element);
}

void QQuickStackViewPrivate::viewItemTransitionFinished(QQuickItemViewTransitionableItem *transitionable)
{
    QQuickStackElement *element = static_cast<QQuickStackElement *>(transitionable);
    if (element->status == QQuickStackView::Activating) {
        element->setStatus(QQuickStackView::Active);
    } else if (element->status == QQuickStackView::Deactivating) {
        element->setStatus(QQuickStackView::Inactive);
        QQuickStackElement *existingElement = element->item ? findElement(element->item) : nullptr;
        // If a different element with the same item is found,
        // do not call setVisible(false) since it needs to be visible.
        if (!existingElement || element == existingElement)
            element->setVisible(false);
        if (element->removal || element->isPendingRemoval())
            removed += element;
    }

    if (transitioner && transitioner->runningJobs.isEmpty()) {
        // ~QQuickStackElement() emits QQuickStackViewAttached::removed(), which may be used
        // to modify the stack. Set the status first and make a copy of the destroyable stack
        // elements to exclude any modifications that may happen during qDeleteAll(). (QTBUG-62153)
        setBusy(false);
        QList<QQuickStackElement*> removedElements = removed;
        removed.clear();

        for (QQuickStackElement *removedElement : std::as_const(removedElements)) {
            // If an element with the same item is found in the active stack list,
            // forget about the item so that we don't hide it.
            if (removedElement->item && findElement(removedElement->item)) {
                QQuickItemPrivate::get(removedElement->item)->removeItemChangeListener(removedElement, QQuickItemPrivate::Destroyed);
                removedElement->item = nullptr;
            }
        }

        qDeleteAll(removedElements);
    }

    removing.remove(element);
}
#endif

void QQuickStackViewPrivate::setBusy(bool b)
{
    Q_Q(QQuickStackView);
    if (busy == b)
        return;

    busy = b;
    q->setFiltersChildMouseEvents(busy);
    emit q->busyChanged();
}

void QQuickStackViewPrivate::depthChange(int newDepth, int oldDepth)
{
    Q_Q(QQuickStackView);
    if (newDepth == oldDepth)
        return;

    emit q->depthChanged();
    if (newDepth == 0 || oldDepth == 0)
        emit q->emptyChanged();
}

QT_END_NAMESPACE
