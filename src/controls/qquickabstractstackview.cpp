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

#include "qquickabstractstackview_p.h"
#include "qquickabstractcontainer_p_p.h"

QT_BEGIN_NAMESPACE

/*!
    \qmltype StackView
    \inherits Container
    \instantiates QQuickAbstractStackView
    \inqmlmodule QtQuick.Controls
    \ingroup navigation
    \brief A stack view control.

    TODO
*/

class QQuickStackElement
{
    QQuickStackElement() : ownItem(false), item(Q_NULLPTR), ownComponent(false), component(Q_NULLPTR) { }

public:
    static QQuickStackElement *fromString(const QString &str, QQmlEngine *engine, QObject *parent)
    {
        QQuickStackElement *element = new QQuickStackElement;
        element->component = new QQmlComponent(engine, QUrl(str), parent);
        element->ownComponent = true;
        return element;
    }

    static QQuickStackElement *fromObject(QObject *object)
    {
        QQuickStackElement *element = new QQuickStackElement;
        element->component = qobject_cast<QQmlComponent *>(object);
        element->item = qobject_cast<QQuickItem *>(object);
        return element;
    }

    bool ownItem;
    QQuickItem *item;

    bool ownComponent;
    QQmlComponent *component;
};

class QQuickAbstractStackViewPrivate : public QQuickAbstractContainerPrivate
{
    Q_DECLARE_PUBLIC(QQuickAbstractStackView)

public:
    QQuickAbstractStackViewPrivate() : busy(false), depth(0), currentItem(Q_NULLPTR) { }

    void setBusy(bool busy);
    void setDepth(int depth);
    void setCurrentItem(QQuickItem *item);

    QList<QQuickStackElement *> createElements(const QV4::ScopedValue &value);
    QQuickItem *pushElements(const QList<QQuickStackElement *> &elements, QQuickAbstractStackView::Operation operation);

    bool busy;
    int depth;
    QVariant initialItem;
    QQuickItem *currentItem;
    QStack<QQuickStackElement *> elements;
};

void QQuickAbstractStackViewPrivate::setBusy(bool value)
{
    Q_Q(QQuickAbstractStackView);
    if (busy != value) {
        busy = value;
        emit q->busyChanged();
    }
}

void QQuickAbstractStackViewPrivate::setDepth(int value)
{
    Q_Q(QQuickAbstractStackView);
    if (depth != value) {
        depth = value;
        emit q->depthChanged();
    }
}

void QQuickAbstractStackViewPrivate::setCurrentItem(QQuickItem *item)
{
    Q_Q(QQuickAbstractStackView);
    if (currentItem != item) {
        currentItem = item;
        emit q->currentItemChanged();
    }
}

QList<QQuickStackElement *> QQuickAbstractStackViewPrivate::createElements(const QV4::ScopedValue &value)
{
    Q_Q(QQuickAbstractStackView);
    QList<QQuickStackElement *> elements;
    if (QV4::String *s = value->asString()) {
        qDebug() << "### STRING:" << s->toQString();
        elements += QQuickStackElement::fromString(s->toQString(), qmlEngine(q), q);
    } else if (QV4::ArrayObject *a = value->asArrayObject()) {
        int len = a->getLength();
        qDebug() << "### ARRAY:" << len;
        for (int i = 0; i < len; ++i) {
            QV4::Scope scope(a->engine());
            QV4::ScopedValue v(scope, a->getIndexed(i));
            elements += createElements(v);
        }
    } else if (QV4::QObjectWrapper *o =value->as<QV4::QObjectWrapper>()) {
        qDebug() << "### QOBJECT:" << o->object();
        elements += QQuickStackElement::fromObject(o->object());
    } else {
        qDebug("### UNKNOWN");
    }
    return elements;
}

QQuickItem *QQuickAbstractStackViewPrivate::pushElements(const QList<QQuickStackElement *> &elems, QQuickAbstractStackView::Operation operation)
{
    Q_Q(QQuickAbstractStackView);
    Q_UNUSED(operation); // TODO
    if (!elems.isEmpty()) {
        foreach (QQuickStackElement *elem, elems) {
            elements.push(elem);
        }
        emit q->depthChanged();
        // TODO: load
        return elems.last()->item;
    }
    return Q_NULLPTR;
}

QQuickAbstractStackView::QQuickAbstractStackView(QQuickItem *parent) :
    QQuickAbstractContainer(*(new QQuickAbstractStackViewPrivate), parent)
{
    setFlag(ItemIsFocusScope);
}

QQuickAbstractStackView::~QQuickAbstractStackView()
{
    Q_D(QQuickAbstractStackView);
    qDeleteAll(d->elements);
}

/*!
    \qmlproperty bool QtQuickControls2::StackView::busy
    \readonly
    \c true if a transition is running, and \c false otherwise.
*/
bool QQuickAbstractStackView::busy() const
{
    Q_D(const QQuickAbstractStackView);
    return d->busy;
}

// TODO: remove
void QQuickAbstractStackView::setBusy(bool busy)
{
    Q_D(QQuickAbstractStackView);
    d->setBusy(busy);
}

/*!
    \qmlproperty int QtQuickControls2::StackView::depth
    \readonly
    The number of items currently pushed onto the stack.
*/
int QQuickAbstractStackView::depth() const
{
    Q_D(const QQuickAbstractStackView);
    return d->depth;
}

// TODO: remove
void QQuickAbstractStackView::setDepth(int depth)
{
    Q_D(QQuickAbstractStackView);
    d->setDepth(depth);
}

/*!
    \qmlproperty Item QtQuickControls2::StackView::currentItem
    \readonly
    The currently top-most item in the stack.
*/
QQuickItem *QQuickAbstractStackView::currentItem() const
{
    Q_D(const QQuickAbstractStackView);
    return d->currentItem;
}

// TODO: remove
void QQuickAbstractStackView::setCurrentItem(QQuickItem *item)
{
    Q_D(QQuickAbstractStackView);
    d->setCurrentItem(item);
}

QQuickItem *QQuickAbstractStackView::qpush(QQmlV4Function *args)
{
    Q_D(QQuickAbstractStackView);
    QV4::ExecutionEngine *v4 = args->v4engine();
    QV4::Scope scope(v4);

    Operation operation = d->elements.isEmpty() ? Immediate : Transition;
    QList<QQuickStackElement *> elements;
    for (int i = 0; i < args->length(); ++i) {
        QV4::ScopedValue value(scope, (*args)[i]);
        if (value->isInt32())
            operation = static_cast<Operation>(value->toInt32());
        else
            elements += d->createElements(value);
    }
    return d->pushElements(elements, operation);
}

QQuickItem *QQuickAbstractStackView::qpop(QQmlV4Function *args)
{
    Q_UNUSED(args); // TODO
    return Q_NULLPTR;
}

void QQuickAbstractStackView::qclear()
{
    // TODO
}

/*!
    \qmlproperty var QtQuickControls2::StackView::initialItem

    The first \l item that should be shown when the StackView is created.
    \a initialItem can take same value as the first argument to \l{StackView::push()}
    {StackView.push()}. Note that this is just a convenience for writing
    \c{Component.onCompleted: stackView.push(myInitialItem)}

    Examples:

    \list
    \li initialItem: Qt.resolvedUrl("MyItem.qml")
    \li initialItem: myItem
    \li initialItem: {"item" : Qt.resolvedUrl("MyRectangle.qml"), "properties" : {"color" : "red"}}
    \endlist
    \sa push
*/
QVariant QQuickAbstractStackView::initialItem() const
{
    Q_D(const QQuickAbstractStackView);
    return d->initialItem;
}

void QQuickAbstractStackView::setInitialItem(const QVariant &item)
{
    Q_D(QQuickAbstractStackView);
    d->initialItem = item;
}

void QQuickAbstractStackView::componentComplete()
{
    QQuickAbstractContainer::componentComplete();

    Q_D(QQuickAbstractStackView);
    if (QObject *o = d->initialItem.value<QObject *>())
        d->pushElements(QList<QQuickStackElement *>() << QQuickStackElement::fromObject(o), Immediate);
    else if (d->initialItem.canConvert<QString>())
        d->pushElements(QList<QQuickStackElement *>() << QQuickStackElement::fromString(d->initialItem.toString(), qmlEngine(this), this), Immediate);
}

/*!
    \qmltype Stack
    \inherits QtObject
    \instantiates QQuickStackAttached
    \inqmlmodule QtQuick.Controls
    \ingroup navigation
    \brief TODO

    TODO
*/

class QQuickStackAttachedPrivate : public QObjectPrivate
{
public:
    QQuickStackAttachedPrivate() : status(QQuickStackAttached::Inactive) { }

    QQuickStackAttached::Status status;
};

QQuickStackAttached::QQuickStackAttached(QObject *parent) :
    QObject(*(new QQuickStackAttachedPrivate), parent)
{
}

QQuickStackAttached *QQuickStackAttached::qmlAttachedProperties(QObject *object)
{
    return new QQuickStackAttached(object);
}

/*!
    \qmlattachedproperty enumeration QtQuickControls2::Stack::status

    TODO
*/
QQuickStackAttached::Status QQuickStackAttached::status() const
{
    Q_D(const QQuickStackAttached);
    return d->status;
}

void QQuickStackAttached::setStatus(Status status)
{
    Q_D(QQuickStackAttached);
    if (d->status != status) {
        d->status = status;
        emit statusChanged();
    }
}

QT_END_NAMESPACE
