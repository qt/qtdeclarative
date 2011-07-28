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

#include "QtQuick1/private/qdeclarativestateoperations_p.h"
#include "QtQuick1/private/qdeclarativeanchors_p_p.h"
#include "QtQuick1/private/qdeclarativeitem_p.h"
#include "QtDeclarative/private/qdeclarativenullablevalue_p_p.h"

#include <QtDeclarative/qdeclarative.h>
#include <QtDeclarative/qdeclarativecontext.h>
#include <QtDeclarative/qdeclarativeexpression.h>
#include <QtDeclarative/qdeclarativeinfo.h>
#include <QtDeclarative/private/qdeclarativeguard_p.h>
#include <QtDeclarative/private/qdeclarativecontext_p.h>
#include <QtDeclarative/private/qdeclarativeproperty_p.h>
#include <QtDeclarative/private/qdeclarativebinding_p.h>
#include <QtQuick1/private/qdeclarativestate_p_p.h>

#include <QtCore/qdebug.h>
#include <QtWidgets/qgraphicsitem.h>
#include <QtCore/qmath.h>

#include <private/qobject_p.h>

QT_BEGIN_NAMESPACE



class QDeclarative1ParentChangePrivate : public QDeclarative1StateOperationPrivate
{
    Q_DECLARE_PUBLIC(QDeclarative1ParentChange)
public:
    QDeclarative1ParentChangePrivate() : target(0), parent(0), origParent(0), origStackBefore(0),
        rewindParent(0), rewindStackBefore(0) {}

    QDeclarativeItem *target;
    QDeclarativeGuard<QDeclarativeItem> parent;
    QDeclarativeGuard<QDeclarativeItem> origParent;
    QDeclarativeGuard<QDeclarativeItem> origStackBefore;
    QDeclarativeItem *rewindParent;
    QDeclarativeItem *rewindStackBefore;

    QDeclarativeNullableValue<QDeclarativeScriptString> xString;
    QDeclarativeNullableValue<QDeclarativeScriptString> yString;
    QDeclarativeNullableValue<QDeclarativeScriptString> widthString;
    QDeclarativeNullableValue<QDeclarativeScriptString> heightString;
    QDeclarativeNullableValue<QDeclarativeScriptString> scaleString;
    QDeclarativeNullableValue<QDeclarativeScriptString> rotationString;

    QDeclarativeNullableValue<qreal> x;
    QDeclarativeNullableValue<qreal> y;
    QDeclarativeNullableValue<qreal> width;
    QDeclarativeNullableValue<qreal> height;
    QDeclarativeNullableValue<qreal> scale;
    QDeclarativeNullableValue<qreal> rotation;

    void doChange(QDeclarativeItem *targetParent, QDeclarativeItem *stackBefore = 0);
};

void QDeclarative1ParentChangePrivate::doChange(QDeclarativeItem *targetParent, QDeclarativeItem *stackBefore)
{
    if (targetParent && target && target->parentItem()) {
        Q_Q(QDeclarative1ParentChange);
        bool ok;
        const QTransform &transform = target->parentItem()->itemTransform(targetParent, &ok);
        if (transform.type() >= QTransform::TxShear || !ok) {
            qmlInfo(q) << QDeclarative1ParentChange::tr("Unable to preserve appearance under complex transform");
            ok = false;
        }

        qreal scale = 1;
        qreal rotation = 0;
        bool isRotate = (transform.type() == QTransform::TxRotate) || (transform.m11() < 0);
        if (ok && !isRotate) {
            if (transform.m11() == transform.m22())
                scale = transform.m11();
            else {
                qmlInfo(q) << QDeclarative1ParentChange::tr("Unable to preserve appearance under non-uniform scale");
                ok = false;
            }
        } else if (ok && isRotate) {
            if (transform.m11() == transform.m22())
                scale = qSqrt(transform.m11()*transform.m11() + transform.m12()*transform.m12());
            else {
                qmlInfo(q) << QDeclarative1ParentChange::tr("Unable to preserve appearance under non-uniform scale");
                ok = false;
            }

            if (scale != 0)
                rotation = atan2(transform.m12()/scale, transform.m11()/scale) * 180/M_PI;
            else {
                qmlInfo(q) << QDeclarative1ParentChange::tr("Unable to preserve appearance under scale of 0");
                ok = false;
            }
        }

        const QPointF &point = transform.map(QPointF(target->x(),target->y()));
        qreal x = point.x();
        qreal y = point.y();

        // setParentItem will update the transformOriginPoint if needed
        target->setParentItem(targetParent);

        if (ok && target->transformOrigin() != QDeclarativeItem::TopLeft) {
            qreal tempxt = target->transformOriginPoint().x();
            qreal tempyt = target->transformOriginPoint().y();
            QTransform t;
            t.translate(-tempxt, -tempyt);
            t.rotate(rotation);
            t.scale(scale, scale);
            t.translate(tempxt, tempyt);
            const QPointF &offset = t.map(QPointF(0,0));
            x += offset.x();
            y += offset.y();
        }

        if (ok) {
            //qDebug() << x << y << rotation << scale;
            target->setX(x);
            target->setY(y);
            target->setRotation(target->rotation() + rotation);
            target->setScale(target->scale() * scale);
        }
    } else if (target) {
        target->setParentItem(targetParent);
    }

    //restore the original stack position.
    //### if stackBefore has also been reparented this won't work
    if (stackBefore)
        target->stackBefore(stackBefore);
}

/*!
    \preliminary
    \qmlclass ParentChange QDeclarative1ParentChange
    \inqmlmodule QtQuick 1
    \ingroup qml-state-elements
    \brief The ParentChange element allows you to reparent an Item in a state change.

    ParentChange reparents an item while preserving its visual appearance (position, size,
    rotation, and scale) on screen. You can then specify a transition to move/resize/rotate/scale
    the item to its final intended appearance.

    ParentChange can only preserve visual appearance if no complex transforms are involved.
    More specifically, it will not work if the transform property has been set for any
    items involved in the reparenting (i.e. items in the common ancestor tree
    for the original and new parent).

    The example below displays a large red rectangle and a small blue rectangle, side by side. 
    When the \c blueRect is clicked, it changes to the "reparented" state: its parent is changed to \c redRect and it is 
    positioned at (10, 10) within the red rectangle, as specified in the ParentChange.

    \snippet doc/src/snippets/declarative/parentchange.qml 0

    \image parentchange.png

    You can specify at which point in a transition you want a ParentChange to occur by
    using a ParentAnimation.
*/


QDeclarative1ParentChange::QDeclarative1ParentChange(QObject *parent)
    : QDeclarative1StateOperation(*(new QDeclarative1ParentChangePrivate), parent)
{
}

QDeclarative1ParentChange::~QDeclarative1ParentChange()
{
}

/*!
    \qmlproperty real QtQuick1::ParentChange::x
    \qmlproperty real QtQuick1::ParentChange::y
    \qmlproperty real QtQuick1::ParentChange::width
    \qmlproperty real QtQuick1::ParentChange::height
    \qmlproperty real QtQuick1::ParentChange::scale
    \qmlproperty real QtQuick1::ParentChange::rotation
    These properties hold the new position, size, scale, and rotation
    for the item in this state.
*/
QDeclarativeScriptString QDeclarative1ParentChange::x() const
{
    Q_D(const QDeclarative1ParentChange);
    return d->xString.value;
}

void tryReal(QDeclarativeNullableValue<qreal> &value, const QString &string)
{
    bool ok = false;
    qreal realValue = string.toFloat(&ok);
    if (ok)
        value = realValue;
    else
        value.invalidate();
}

void QDeclarative1ParentChange::setX(QDeclarativeScriptString x)
{
    Q_D(QDeclarative1ParentChange);
    d->xString = x;
    tryReal(d->x, x.script());
}

bool QDeclarative1ParentChange::xIsSet() const
{
    Q_D(const QDeclarative1ParentChange);
    return d->xString.isValid();
}

QDeclarativeScriptString QDeclarative1ParentChange::y() const
{
    Q_D(const QDeclarative1ParentChange);
    return d->yString.value;
}

void QDeclarative1ParentChange::setY(QDeclarativeScriptString y)
{
    Q_D(QDeclarative1ParentChange);
    d->yString = y;
    tryReal(d->y, y.script());
}

bool QDeclarative1ParentChange::yIsSet() const
{
    Q_D(const QDeclarative1ParentChange);
    return d->yString.isValid();
}

QDeclarativeScriptString QDeclarative1ParentChange::width() const
{
    Q_D(const QDeclarative1ParentChange);
    return d->widthString.value;
}

void QDeclarative1ParentChange::setWidth(QDeclarativeScriptString width)
{
    Q_D(QDeclarative1ParentChange);
    d->widthString = width;
    tryReal(d->width, width.script());
}

bool QDeclarative1ParentChange::widthIsSet() const
{
    Q_D(const QDeclarative1ParentChange);
    return d->widthString.isValid();
}

QDeclarativeScriptString QDeclarative1ParentChange::height() const
{
    Q_D(const QDeclarative1ParentChange);
    return d->heightString.value;
}

void QDeclarative1ParentChange::setHeight(QDeclarativeScriptString height)
{
    Q_D(QDeclarative1ParentChange);
    d->heightString = height;
    tryReal(d->height, height.script());
}

bool QDeclarative1ParentChange::heightIsSet() const
{
    Q_D(const QDeclarative1ParentChange);
    return d->heightString.isValid();
}

QDeclarativeScriptString QDeclarative1ParentChange::scale() const
{
    Q_D(const QDeclarative1ParentChange);
    return d->scaleString.value;
}

void QDeclarative1ParentChange::setScale(QDeclarativeScriptString scale)
{
    Q_D(QDeclarative1ParentChange);
    d->scaleString = scale;
    tryReal(d->scale, scale.script());
}

bool QDeclarative1ParentChange::scaleIsSet() const
{
    Q_D(const QDeclarative1ParentChange);
    return d->scaleString.isValid();
}

QDeclarativeScriptString QDeclarative1ParentChange::rotation() const
{
    Q_D(const QDeclarative1ParentChange);
    return d->rotationString.value;
}

void QDeclarative1ParentChange::setRotation(QDeclarativeScriptString rotation)
{
    Q_D(QDeclarative1ParentChange);
    d->rotationString = rotation;
    tryReal(d->rotation, rotation.script());
}

bool QDeclarative1ParentChange::rotationIsSet() const
{
    Q_D(const QDeclarative1ParentChange);
    return d->rotationString.isValid();
}

QDeclarativeItem *QDeclarative1ParentChange::originalParent() const
{
    Q_D(const QDeclarative1ParentChange);
    return d->origParent;
}

/*!
    \qmlproperty Item QtQuick1::ParentChange::target
    This property holds the item to be reparented
*/

QDeclarativeItem *QDeclarative1ParentChange::object() const
{
    Q_D(const QDeclarative1ParentChange);
    return d->target;
}

void QDeclarative1ParentChange::setObject(QDeclarativeItem *target)
{
    Q_D(QDeclarative1ParentChange);
    d->target = target;
}

/*!
    \qmlproperty Item QtQuick1::ParentChange::parent
    This property holds the new parent for the item in this state.
*/

QDeclarativeItem *QDeclarative1ParentChange::parent() const
{
    Q_D(const QDeclarative1ParentChange);
    return d->parent;
}

void QDeclarative1ParentChange::setParent(QDeclarativeItem *parent)
{
    Q_D(QDeclarative1ParentChange);
    d->parent = parent;
}

QDeclarative1StateOperation::ActionList QDeclarative1ParentChange::actions()
{
    Q_D(QDeclarative1ParentChange);
    if (!d->target || !d->parent)
        return ActionList();

    ActionList actions;

    QDeclarative1Action a;
    a.event = this;
    actions << a;

    QDeclarativeContext *ctxt = qmlContext(this);

    if (d->xString.isValid()) {
        if (d->x.isValid()) {
            QDeclarative1Action xa(d->target, QLatin1String("x"), ctxt, d->x.value);
            actions << xa;
        } else {
            QDeclarativeBinding *newBinding = new QDeclarativeBinding(d->xString.value.script(), d->target, ctxt);
            newBinding->setTarget(QDeclarativeProperty(d->target, QLatin1String("x"), ctxt));
            QDeclarative1Action xa;
            xa.property = newBinding->property();
            xa.toBinding = newBinding;
            xa.fromValue = xa.property.read();
            xa.deletableToBinding = true;
            actions << xa;
        }
    }

    if (d->yString.isValid()) {
        if (d->y.isValid()) {
            QDeclarative1Action ya(d->target, QLatin1String("y"), ctxt, d->y.value);
            actions << ya;
        } else {
            QDeclarativeBinding *newBinding = new QDeclarativeBinding(d->yString.value.script(), d->target, ctxt);
            newBinding->setTarget(QDeclarativeProperty(d->target, QLatin1String("y"), ctxt));
            QDeclarative1Action ya;
            ya.property = newBinding->property();
            ya.toBinding = newBinding;
            ya.fromValue = ya.property.read();
            ya.deletableToBinding = true;
            actions << ya;
        }
    }

    if (d->scaleString.isValid()) {
        if (d->scale.isValid()) {
            QDeclarative1Action sa(d->target, QLatin1String("scale"), ctxt, d->scale.value);
            actions << sa;
        } else {
            QDeclarativeBinding *newBinding = new QDeclarativeBinding(d->scaleString.value.script(), d->target, ctxt);
            newBinding->setTarget(QDeclarativeProperty(d->target, QLatin1String("scale"), ctxt));
            QDeclarative1Action sa;
            sa.property = newBinding->property();
            sa.toBinding = newBinding;
            sa.fromValue = sa.property.read();
            sa.deletableToBinding = true;
            actions << sa;
        }
    }

    if (d->rotationString.isValid()) {
        if (d->rotation.isValid()) {
            QDeclarative1Action ra(d->target, QLatin1String("rotation"), ctxt, d->rotation.value);
            actions << ra;
        } else {
            QDeclarativeBinding *newBinding = new QDeclarativeBinding(d->rotationString.value.script(), d->target, ctxt);
            newBinding->setTarget(QDeclarativeProperty(d->target, QLatin1String("rotation"), ctxt));
            QDeclarative1Action ra;
            ra.property = newBinding->property();
            ra.toBinding = newBinding;
            ra.fromValue = ra.property.read();
            ra.deletableToBinding = true;
            actions << ra;
        }
    }

    if (d->widthString.isValid()) {
        if (d->width.isValid()) {
            QDeclarative1Action wa(d->target, QLatin1String("width"), ctxt, d->width.value);
            actions << wa;
        } else {
            QDeclarativeBinding *newBinding = new QDeclarativeBinding(d->widthString.value.script(), d->target, ctxt);
            newBinding->setTarget(QDeclarativeProperty(d->target, QLatin1String("width"), ctxt));
            QDeclarative1Action wa;
            wa.property = newBinding->property();
            wa.toBinding = newBinding;
            wa.fromValue = wa.property.read();
            wa.deletableToBinding = true;
            actions << wa;
        }
    }

    if (d->heightString.isValid()) {
        if (d->height.isValid()) {
            QDeclarative1Action ha(d->target, QLatin1String("height"), ctxt, d->height.value);
            actions << ha;
        } else {
            QDeclarativeBinding *newBinding = new QDeclarativeBinding(d->heightString.value.script(), d->target, ctxt);
            newBinding->setTarget(QDeclarativeProperty(d->target, QLatin1String("height"), ctxt));
            QDeclarative1Action ha;
            ha.property = newBinding->property();
            ha.toBinding = newBinding;
            ha.fromValue = ha.property.read();
            ha.deletableToBinding = true;
            actions << ha;
        }
    }

    return actions;
}

class AccessibleFxItem : public QDeclarativeItem
{
    Q_OBJECT
    Q_DECLARE_PRIVATE_D(QGraphicsItem::d_ptr.data(), QDeclarativeItem)
public:
    int siblingIndex() {
        Q_D(QDeclarativeItem);
        return d->siblingIndex;
    }
};

void QDeclarative1ParentChange::saveOriginals()
{
    Q_D(QDeclarative1ParentChange);
    saveCurrentValues();
    d->origParent = d->rewindParent;
    d->origStackBefore = d->rewindStackBefore;
}

/*void QDeclarative1ParentChange::copyOriginals(QDeclarative1ActionEvent *other)
{
    Q_D(QDeclarative1ParentChange);
    QDeclarative1ParentChange *pc = static_cast<QDeclarative1ParentChange*>(other);

    d->origParent = pc->d_func()->rewindParent;
    d->origStackBefore = pc->d_func()->rewindStackBefore;

    saveCurrentValues();
}*/

void QDeclarative1ParentChange::execute(Reason)
{
    Q_D(QDeclarative1ParentChange);
    d->doChange(d->parent);
}

bool QDeclarative1ParentChange::isReversable()
{
    return true;
}

void QDeclarative1ParentChange::reverse(Reason)
{
    Q_D(QDeclarative1ParentChange);
    d->doChange(d->origParent, d->origStackBefore);
}

QString QDeclarative1ParentChange::typeName() const
{
    return QLatin1String("ParentChange");
}

bool QDeclarative1ParentChange::override(QDeclarative1ActionEvent*other)
{
    Q_D(QDeclarative1ParentChange);
    if (other->typeName() != QLatin1String("ParentChange"))
        return false;
    if (QDeclarative1ParentChange *otherPC = static_cast<QDeclarative1ParentChange*>(other))
        return (d->target == otherPC->object());
    return false;
}

void QDeclarative1ParentChange::saveCurrentValues()
{
    Q_D(QDeclarative1ParentChange);
    if (!d->target) {
        d->rewindParent = 0;
        d->rewindStackBefore = 0;
        return;
    }

    d->rewindParent = d->target->parentItem();
    d->rewindStackBefore = 0;

    if (!d->rewindParent)
        return;

    //try to determine the item's original stack position so we can restore it
    int siblingIndex = ((AccessibleFxItem*)d->target)->siblingIndex() + 1;
    QList<QGraphicsItem*> children = d->rewindParent->childItems();
    for (int i = 0; i < children.count(); ++i) {
        QDeclarativeItem *child = qobject_cast<QDeclarativeItem*>(children.at(i));
        if (!child)
            continue;
        if (((AccessibleFxItem*)child)->siblingIndex() == siblingIndex) {
            d->rewindStackBefore = child;
            break;
        }
    }
}

void QDeclarative1ParentChange::rewind()
{
    Q_D(QDeclarative1ParentChange);
    d->doChange(d->rewindParent, d->rewindStackBefore);
}

class QDeclarative1StateChangeScriptPrivate : public QDeclarative1StateOperationPrivate
{
public:
    QDeclarative1StateChangeScriptPrivate() {}

    QDeclarativeScriptString script;
    QString name;
};

/*!
    \qmlclass StateChangeScript QDeclarative1StateChangeScript
    \inqmlmodule QtQuick 1
    \ingroup qml-state-elements
    \brief The StateChangeScript element allows you to run a script in a state.

    A StateChangeScript is run upon entering a state. You can optionally use
    ScriptAction to specify the point in the transition at which
    the StateChangeScript should to be run.

    \snippet snippets/declarative/states/statechangescript.qml state and transition

    \sa ScriptAction
*/

QDeclarative1StateChangeScript::QDeclarative1StateChangeScript(QObject *parent)
: QDeclarative1StateOperation(*(new QDeclarative1StateChangeScriptPrivate), parent)
{
}

QDeclarative1StateChangeScript::~QDeclarative1StateChangeScript()
{
}

/*!
    \qmlproperty script QtQuick1::StateChangeScript::script
    This property holds the script to run when the state is current.
*/
QDeclarativeScriptString QDeclarative1StateChangeScript::script() const
{
    Q_D(const QDeclarative1StateChangeScript);
    return d->script;
}

void QDeclarative1StateChangeScript::setScript(const QDeclarativeScriptString &s)
{
    Q_D(QDeclarative1StateChangeScript);
    d->script = s;
}

/*!
    \qmlproperty string QtQuick1::StateChangeScript::name
    This property holds the name of the script. This name can be used by a
    ScriptAction to target a specific script.

    \sa ScriptAction::scriptName
*/
QString QDeclarative1StateChangeScript::name() const
{
    Q_D(const QDeclarative1StateChangeScript);
    return d->name;
}

void QDeclarative1StateChangeScript::setName(const QString &n)
{
    Q_D(QDeclarative1StateChangeScript);
    d->name = n;
}

void QDeclarative1StateChangeScript::execute(Reason)
{
    Q_D(QDeclarative1StateChangeScript);
    const QString &script = d->script.script();
    if (!script.isEmpty()) {
        QDeclarativeExpression expr(d->script.context(), d->script.scopeObject(), script);
        QDeclarativeData *ddata = QDeclarativeData::get(this);
        if (ddata && ddata->outerContext && !ddata->outerContext->url.isEmpty())
            expr.setSourceLocation(ddata->outerContext->url.toString(), ddata->lineNumber);
        expr.evaluate();
        if (expr.hasError())
            qmlInfo(this, expr.error());
    }
}

QDeclarative1StateChangeScript::ActionList QDeclarative1StateChangeScript::actions()
{
    ActionList rv;
    QDeclarative1Action a;
    a.event = this;
    rv << a;
    return rv;
}

QString QDeclarative1StateChangeScript::typeName() const
{
    return QLatin1String("StateChangeScript");
}

/*!
    \qmlclass AnchorChanges QDeclarative1AnchorChanges
    \inqmlmodule QtQuick 1
    \ingroup qml-state-elements
    \brief The AnchorChanges element allows you to change the anchors of an item in a state.

    The AnchorChanges element is used to modify the anchors of an item in a \l State.

    AnchorChanges cannot be used to modify the margins on an item. For this, use
    PropertyChanges intead.

    In the following example we change the top and bottom anchors of an item
    using AnchorChanges, and the top and bottom anchor margins using
    PropertyChanges:

    \snippet doc/src/snippets/declarative/anchorchanges.qml 0

    \image anchorchanges.png

    AnchorChanges can be animated using AnchorAnimation.
    \qml
    //animate our anchor changes
    Transition {
        AnchorAnimation {}
    }
    \endqml

    Margin animations can be animated using NumberAnimation.

    For more information on anchors see \l {anchor-layout}{Anchor Layouts}.
*/

class QDeclarative1AnchorSetPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QDeclarative1AnchorSet)
public:
    QDeclarative1AnchorSetPrivate()
      : usedAnchors(0), resetAnchors(0), fill(0),
        centerIn(0)/*, leftMargin(0), rightMargin(0), topMargin(0), bottomMargin(0),
        margins(0), vCenterOffset(0), hCenterOffset(0), baselineOffset(0)*/
    {
    }

    QDeclarative1Anchors::Anchors usedAnchors;
    QDeclarative1Anchors::Anchors resetAnchors;

    QDeclarativeItem *fill;
    QDeclarativeItem *centerIn;

    QDeclarativeScriptString leftScript;
    QDeclarativeScriptString rightScript;
    QDeclarativeScriptString topScript;
    QDeclarativeScriptString bottomScript;
    QDeclarativeScriptString hCenterScript;
    QDeclarativeScriptString vCenterScript;
    QDeclarativeScriptString baselineScript;

    /*qreal leftMargin;
    qreal rightMargin;
    qreal topMargin;
    qreal bottomMargin;
    qreal margins;
    qreal vCenterOffset;
    qreal hCenterOffset;
    qreal baselineOffset;*/
};

QDeclarative1AnchorSet::QDeclarative1AnchorSet(QObject *parent)
  : QObject(*new QDeclarative1AnchorSetPrivate, parent)
{
}

QDeclarative1AnchorSet::~QDeclarative1AnchorSet()
{
}

QDeclarativeScriptString QDeclarative1AnchorSet::top() const
{
    Q_D(const QDeclarative1AnchorSet);
    return d->topScript;
}

void QDeclarative1AnchorSet::setTop(const QDeclarativeScriptString &edge)
{
    Q_D(QDeclarative1AnchorSet);
    d->usedAnchors |= QDeclarative1Anchors::TopAnchor;
    d->topScript = edge;
    if (edge.script() == QLatin1String("undefined"))
        resetTop();
}

void QDeclarative1AnchorSet::resetTop()
{
    Q_D(QDeclarative1AnchorSet);
    d->usedAnchors &= ~QDeclarative1Anchors::TopAnchor;
    d->topScript = QDeclarativeScriptString();
    d->resetAnchors |= QDeclarative1Anchors::TopAnchor;
}

QDeclarativeScriptString QDeclarative1AnchorSet::bottom() const
{
    Q_D(const QDeclarative1AnchorSet);
    return d->bottomScript;
}

void QDeclarative1AnchorSet::setBottom(const QDeclarativeScriptString &edge)
{
    Q_D(QDeclarative1AnchorSet);
    d->usedAnchors |= QDeclarative1Anchors::BottomAnchor;
    d->bottomScript = edge;
    if (edge.script() == QLatin1String("undefined"))
        resetBottom();
}

void QDeclarative1AnchorSet::resetBottom()
{
    Q_D(QDeclarative1AnchorSet);
    d->usedAnchors &= ~QDeclarative1Anchors::BottomAnchor;
    d->bottomScript = QDeclarativeScriptString();
    d->resetAnchors |= QDeclarative1Anchors::BottomAnchor;
}

QDeclarativeScriptString QDeclarative1AnchorSet::verticalCenter() const
{
    Q_D(const QDeclarative1AnchorSet);
    return d->vCenterScript;
}

void QDeclarative1AnchorSet::setVerticalCenter(const QDeclarativeScriptString &edge)
{
    Q_D(QDeclarative1AnchorSet);
    d->usedAnchors |= QDeclarative1Anchors::VCenterAnchor;
    d->vCenterScript = edge;
    if (edge.script() == QLatin1String("undefined"))
        resetVerticalCenter();
}

void QDeclarative1AnchorSet::resetVerticalCenter()
{
    Q_D(QDeclarative1AnchorSet);
    d->usedAnchors &= ~QDeclarative1Anchors::VCenterAnchor;
    d->vCenterScript = QDeclarativeScriptString();
    d->resetAnchors |= QDeclarative1Anchors::VCenterAnchor;
}

QDeclarativeScriptString QDeclarative1AnchorSet::baseline() const
{
    Q_D(const QDeclarative1AnchorSet);
    return d->baselineScript;
}

void QDeclarative1AnchorSet::setBaseline(const QDeclarativeScriptString &edge)
{
    Q_D(QDeclarative1AnchorSet);
    d->usedAnchors |= QDeclarative1Anchors::BaselineAnchor;
    d->baselineScript = edge;
    if (edge.script() == QLatin1String("undefined"))
        resetBaseline();
}

void QDeclarative1AnchorSet::resetBaseline()
{
    Q_D(QDeclarative1AnchorSet);
    d->usedAnchors &= ~QDeclarative1Anchors::BaselineAnchor;
    d->baselineScript = QDeclarativeScriptString();
    d->resetAnchors |= QDeclarative1Anchors::BaselineAnchor;
}

QDeclarativeScriptString QDeclarative1AnchorSet::left() const
{
    Q_D(const QDeclarative1AnchorSet);
    return d->leftScript;
}

void QDeclarative1AnchorSet::setLeft(const QDeclarativeScriptString &edge)
{
    Q_D(QDeclarative1AnchorSet);
    d->usedAnchors |= QDeclarative1Anchors::LeftAnchor;
    d->leftScript = edge;
    if (edge.script() == QLatin1String("undefined"))
        resetLeft();
}

void QDeclarative1AnchorSet::resetLeft()
{
    Q_D(QDeclarative1AnchorSet);
    d->usedAnchors &= ~QDeclarative1Anchors::LeftAnchor;
    d->leftScript = QDeclarativeScriptString();
    d->resetAnchors |= QDeclarative1Anchors::LeftAnchor;
}

QDeclarativeScriptString QDeclarative1AnchorSet::right() const
{
    Q_D(const QDeclarative1AnchorSet);
    return d->rightScript;
}

void QDeclarative1AnchorSet::setRight(const QDeclarativeScriptString &edge)
{
    Q_D(QDeclarative1AnchorSet);
    d->usedAnchors |= QDeclarative1Anchors::RightAnchor;
    d->rightScript = edge;
    if (edge.script() == QLatin1String("undefined"))
        resetRight();
}

void QDeclarative1AnchorSet::resetRight()
{
    Q_D(QDeclarative1AnchorSet);
    d->usedAnchors &= ~QDeclarative1Anchors::RightAnchor;
    d->rightScript = QDeclarativeScriptString();
    d->resetAnchors |= QDeclarative1Anchors::RightAnchor;
}

QDeclarativeScriptString QDeclarative1AnchorSet::horizontalCenter() const
{
    Q_D(const QDeclarative1AnchorSet);
    return d->hCenterScript;
}

void QDeclarative1AnchorSet::setHorizontalCenter(const QDeclarativeScriptString &edge)
{
    Q_D(QDeclarative1AnchorSet);
    d->usedAnchors |= QDeclarative1Anchors::HCenterAnchor;
    d->hCenterScript = edge;
    if (edge.script() == QLatin1String("undefined"))
        resetHorizontalCenter();
}

void QDeclarative1AnchorSet::resetHorizontalCenter()
{
    Q_D(QDeclarative1AnchorSet);
    d->usedAnchors &= ~QDeclarative1Anchors::HCenterAnchor;
    d->hCenterScript = QDeclarativeScriptString();
    d->resetAnchors |= QDeclarative1Anchors::HCenterAnchor;
}

QDeclarativeItem *QDeclarative1AnchorSet::fill() const
{
    Q_D(const QDeclarative1AnchorSet);
    return d->fill;
}

void QDeclarative1AnchorSet::setFill(QDeclarativeItem *f)
{
    Q_D(QDeclarative1AnchorSet);
    d->fill = f;
}

void QDeclarative1AnchorSet::resetFill()
{
    setFill(0);
}

QDeclarativeItem *QDeclarative1AnchorSet::centerIn() const
{
    Q_D(const QDeclarative1AnchorSet);
    return d->centerIn;
}

void QDeclarative1AnchorSet::setCenterIn(QDeclarativeItem* c)
{
    Q_D(QDeclarative1AnchorSet);
    d->centerIn = c;
}

void QDeclarative1AnchorSet::resetCenterIn()
{
    setCenterIn(0);
}


class QDeclarative1AnchorChangesPrivate : public QDeclarative1StateOperationPrivate
{
public:
    QDeclarative1AnchorChangesPrivate()
        : target(0), anchorSet(new QDeclarative1AnchorSet),
          leftBinding(0), rightBinding(0), hCenterBinding(0),
          topBinding(0), bottomBinding(0), vCenterBinding(0), baselineBinding(0),
          origLeftBinding(0), origRightBinding(0), origHCenterBinding(0),
          origTopBinding(0), origBottomBinding(0), origVCenterBinding(0),
          origBaselineBinding(0)
    {

    }
    ~QDeclarative1AnchorChangesPrivate() { delete anchorSet; }

    QDeclarativeItem *target;
    QDeclarative1AnchorSet *anchorSet;

    QDeclarativeBinding *leftBinding;
    QDeclarativeBinding *rightBinding;
    QDeclarativeBinding *hCenterBinding;
    QDeclarativeBinding *topBinding;
    QDeclarativeBinding *bottomBinding;
    QDeclarativeBinding *vCenterBinding;
    QDeclarativeBinding *baselineBinding;

    QDeclarativeAbstractBinding *origLeftBinding;
    QDeclarativeAbstractBinding *origRightBinding;
    QDeclarativeAbstractBinding *origHCenterBinding;
    QDeclarativeAbstractBinding *origTopBinding;
    QDeclarativeAbstractBinding *origBottomBinding;
    QDeclarativeAbstractBinding *origVCenterBinding;
    QDeclarativeAbstractBinding *origBaselineBinding;

    QDeclarative1AnchorLine rewindLeft;
    QDeclarative1AnchorLine rewindRight;
    QDeclarative1AnchorLine rewindHCenter;
    QDeclarative1AnchorLine rewindTop;
    QDeclarative1AnchorLine rewindBottom;
    QDeclarative1AnchorLine rewindVCenter;
    QDeclarative1AnchorLine rewindBaseline;

    qreal fromX;
    qreal fromY;
    qreal fromWidth;
    qreal fromHeight;

    qreal toX;
    qreal toY;
    qreal toWidth;
    qreal toHeight;

    qreal rewindX;
    qreal rewindY;
    qreal rewindWidth;
    qreal rewindHeight;

    bool applyOrigLeft;
    bool applyOrigRight;
    bool applyOrigHCenter;
    bool applyOrigTop;
    bool applyOrigBottom;
    bool applyOrigVCenter;
    bool applyOrigBaseline;

    QDeclarativeNullableValue<qreal> origWidth;
    QDeclarativeNullableValue<qreal> origHeight;
    qreal origX;
    qreal origY;

    QList<QDeclarativeAbstractBinding*> oldBindings;

    QDeclarativeProperty leftProp;
    QDeclarativeProperty rightProp;
    QDeclarativeProperty hCenterProp;
    QDeclarativeProperty topProp;
    QDeclarativeProperty bottomProp;
    QDeclarativeProperty vCenterProp;
    QDeclarativeProperty baselineProp;
};

/*!
    \qmlproperty Item QtQuick1::AnchorChanges::target
    This property holds the \l Item for which the anchor changes will be applied.
*/

QDeclarative1AnchorChanges::QDeclarative1AnchorChanges(QObject *parent)
 : QDeclarative1StateOperation(*(new QDeclarative1AnchorChangesPrivate), parent)
{
}

QDeclarative1AnchorChanges::~QDeclarative1AnchorChanges()
{
}

QDeclarative1AnchorChanges::ActionList QDeclarative1AnchorChanges::actions()
{
    Q_D(QDeclarative1AnchorChanges);
    d->leftBinding = d->rightBinding = d->hCenterBinding = d->topBinding
                   = d->bottomBinding = d->vCenterBinding = d->baselineBinding = 0;

    d->leftProp = QDeclarativeProperty(d->target, QLatin1String("anchors.left"));
    d->rightProp = QDeclarativeProperty(d->target, QLatin1String("anchors.right"));
    d->hCenterProp = QDeclarativeProperty(d->target, QLatin1String("anchors.horizontalCenter"));
    d->topProp = QDeclarativeProperty(d->target, QLatin1String("anchors.top"));
    d->bottomProp = QDeclarativeProperty(d->target, QLatin1String("anchors.bottom"));
    d->vCenterProp = QDeclarativeProperty(d->target, QLatin1String("anchors.verticalCenter"));
    d->baselineProp = QDeclarativeProperty(d->target, QLatin1String("anchors.baseline"));

    QDeclarativeContext *ctxt = qmlContext(this);

    if (d->anchorSet->d_func()->usedAnchors & QDeclarative1Anchors::LeftAnchor) {
        d->leftBinding = new QDeclarativeBinding(d->anchorSet->d_func()->leftScript.script(), d->target, ctxt);
        d->leftBinding->setTarget(d->leftProp);
    }
    if (d->anchorSet->d_func()->usedAnchors & QDeclarative1Anchors::RightAnchor) {
        d->rightBinding = new QDeclarativeBinding(d->anchorSet->d_func()->rightScript.script(), d->target, ctxt);
        d->rightBinding->setTarget(d->rightProp);
    }
    if (d->anchorSet->d_func()->usedAnchors & QDeclarative1Anchors::HCenterAnchor) {
        d->hCenterBinding = new QDeclarativeBinding(d->anchorSet->d_func()->hCenterScript.script(), d->target, ctxt);
        d->hCenterBinding->setTarget(d->hCenterProp);
    }
    if (d->anchorSet->d_func()->usedAnchors & QDeclarative1Anchors::TopAnchor) {
        d->topBinding = new QDeclarativeBinding(d->anchorSet->d_func()->topScript.script(), d->target, ctxt);
        d->topBinding->setTarget(d->topProp);
    }
    if (d->anchorSet->d_func()->usedAnchors & QDeclarative1Anchors::BottomAnchor) {
        d->bottomBinding = new QDeclarativeBinding(d->anchorSet->d_func()->bottomScript.script(), d->target, ctxt);
        d->bottomBinding->setTarget(d->bottomProp);
    }
    if (d->anchorSet->d_func()->usedAnchors & QDeclarative1Anchors::VCenterAnchor) {
        d->vCenterBinding = new QDeclarativeBinding(d->anchorSet->d_func()->vCenterScript.script(), d->target, ctxt);
        d->vCenterBinding->setTarget(d->vCenterProp);
    }
    if (d->anchorSet->d_func()->usedAnchors & QDeclarative1Anchors::BaselineAnchor) {
        d->baselineBinding = new QDeclarativeBinding(d->anchorSet->d_func()->baselineScript.script(), d->target, ctxt);
        d->baselineBinding->setTarget(d->baselineProp);
    }

    QDeclarative1Action a;
    a.event = this;
    return ActionList() << a;
}

QDeclarative1AnchorSet *QDeclarative1AnchorChanges::anchors()
{
    Q_D(QDeclarative1AnchorChanges);
    return d->anchorSet;
}

QDeclarativeItem *QDeclarative1AnchorChanges::object() const
{
    Q_D(const QDeclarative1AnchorChanges);
    return d->target;
}

void QDeclarative1AnchorChanges::setObject(QDeclarativeItem *target)
{
    Q_D(QDeclarative1AnchorChanges);
    d->target = target;
}

/*!
    \qmlproperty AnchorLine QtQuick1::AnchorChanges::anchors.left
    \qmlproperty AnchorLine QtQuick1::AnchorChanges::anchors.right
    \qmlproperty AnchorLine QtQuick1::AnchorChanges::anchors.horizontalCenter
    \qmlproperty AnchorLine QtQuick1::AnchorChanges::anchors.top
    \qmlproperty AnchorLine QtQuick1::AnchorChanges::anchors.bottom
    \qmlproperty AnchorLine QtQuick1::AnchorChanges::anchors.verticalCenter
    \qmlproperty AnchorLine QtQuick1::AnchorChanges::anchors.baseline

    These properties change the respective anchors of the item.

    To reset an anchor you can assign \c undefined:
    \qml
    AnchorChanges {
        target: myItem
        anchors.left: undefined          //remove myItem's left anchor
        anchors.right: otherItem.right
    }
    \endqml
*/

void QDeclarative1AnchorChanges::execute(Reason reason)
{
    Q_D(QDeclarative1AnchorChanges);
    if (!d->target)
        return;

    QDeclarativeItemPrivate *targetPrivate = QDeclarativeItemPrivate::get(d->target);
    //incorporate any needed "reverts"
    if (d->applyOrigLeft) {
        if (!d->origLeftBinding)
            targetPrivate->anchors()->resetLeft();
        QDeclarativePropertyPrivate::setBinding(d->leftProp, d->origLeftBinding);
    }
    if (d->applyOrigRight) {
        if (!d->origRightBinding)
            targetPrivate->anchors()->resetRight();
        QDeclarativePropertyPrivate::setBinding(d->rightProp, d->origRightBinding);
    }
    if (d->applyOrigHCenter) {
        if (!d->origHCenterBinding)
            targetPrivate->anchors()->resetHorizontalCenter();
        QDeclarativePropertyPrivate::setBinding(d->hCenterProp, d->origHCenterBinding);
    }
    if (d->applyOrigTop) {
        if (!d->origTopBinding)
            targetPrivate->anchors()->resetTop();
        QDeclarativePropertyPrivate::setBinding(d->topProp, d->origTopBinding);
    }
    if (d->applyOrigBottom) {
        if (!d->origBottomBinding)
            targetPrivate->anchors()->resetBottom();
        QDeclarativePropertyPrivate::setBinding(d->bottomProp, d->origBottomBinding);
    }
    if (d->applyOrigVCenter) {
        if (!d->origVCenterBinding)
            targetPrivate->anchors()->resetVerticalCenter();
        QDeclarativePropertyPrivate::setBinding(d->vCenterProp, d->origVCenterBinding);
    }
    if (d->applyOrigBaseline) {
        if (!d->origBaselineBinding)
            targetPrivate->anchors()->resetBaseline();
        QDeclarativePropertyPrivate::setBinding(d->baselineProp, d->origBaselineBinding);
    }

    //destroy old bindings
    if (reason == ActualChange) {
        for (int i = 0; i < d->oldBindings.size(); ++i) {
            QDeclarativeAbstractBinding *binding = d->oldBindings.at(i);
            if (binding)
                binding->destroy();
        }
        d->oldBindings.clear();
    }

    //reset any anchors that have been specified as "undefined"
    if (d->anchorSet->d_func()->resetAnchors & QDeclarative1Anchors::LeftAnchor) {
        targetPrivate->anchors()->resetLeft();
        QDeclarativePropertyPrivate::setBinding(d->leftProp, 0);
    }
    if (d->anchorSet->d_func()->resetAnchors & QDeclarative1Anchors::RightAnchor) {
        targetPrivate->anchors()->resetRight();
        QDeclarativePropertyPrivate::setBinding(d->rightProp, 0);
    }
    if (d->anchorSet->d_func()->resetAnchors & QDeclarative1Anchors::HCenterAnchor) {
        targetPrivate->anchors()->resetHorizontalCenter();
        QDeclarativePropertyPrivate::setBinding(d->hCenterProp, 0);
    }
    if (d->anchorSet->d_func()->resetAnchors & QDeclarative1Anchors::TopAnchor) {
        targetPrivate->anchors()->resetTop();
        QDeclarativePropertyPrivate::setBinding(d->topProp, 0);
    }
    if (d->anchorSet->d_func()->resetAnchors & QDeclarative1Anchors::BottomAnchor) {
        targetPrivate->anchors()->resetBottom();
        QDeclarativePropertyPrivate::setBinding(d->bottomProp, 0);
    }
    if (d->anchorSet->d_func()->resetAnchors & QDeclarative1Anchors::VCenterAnchor) {
        targetPrivate->anchors()->resetVerticalCenter();
        QDeclarativePropertyPrivate::setBinding(d->vCenterProp, 0);
    }
    if (d->anchorSet->d_func()->resetAnchors & QDeclarative1Anchors::BaselineAnchor) {
        targetPrivate->anchors()->resetBaseline();
        QDeclarativePropertyPrivate::setBinding(d->baselineProp, 0);
    }

    //set any anchors that have been specified
    if (d->leftBinding)
        QDeclarativePropertyPrivate::setBinding(d->leftBinding->property(), d->leftBinding);
    if (d->rightBinding)
        QDeclarativePropertyPrivate::setBinding(d->rightBinding->property(), d->rightBinding);
    if (d->hCenterBinding)
        QDeclarativePropertyPrivate::setBinding(d->hCenterBinding->property(), d->hCenterBinding);
    if (d->topBinding)
        QDeclarativePropertyPrivate::setBinding(d->topBinding->property(), d->topBinding);
    if (d->bottomBinding)
        QDeclarativePropertyPrivate::setBinding(d->bottomBinding->property(), d->bottomBinding);
    if (d->vCenterBinding)
        QDeclarativePropertyPrivate::setBinding(d->vCenterBinding->property(), d->vCenterBinding);
    if (d->baselineBinding)
        QDeclarativePropertyPrivate::setBinding(d->baselineBinding->property(), d->baselineBinding);
}

bool QDeclarative1AnchorChanges::isReversable()
{
    return true;
}

void QDeclarative1AnchorChanges::reverse(Reason reason)
{
    Q_D(QDeclarative1AnchorChanges);
    if (!d->target)
        return;

    QDeclarativeItemPrivate *targetPrivate = QDeclarativeItemPrivate::get(d->target);
    //reset any anchors set by the state
    if (d->leftBinding) {
        targetPrivate->anchors()->resetLeft();
        QDeclarativePropertyPrivate::setBinding(d->leftBinding->property(), 0);
        if (reason == ActualChange) {
            d->leftBinding->destroy(); d->leftBinding = 0;
        }
    }
    if (d->rightBinding) {
        targetPrivate->anchors()->resetRight();
        QDeclarativePropertyPrivate::setBinding(d->rightBinding->property(), 0);
        if (reason == ActualChange) {
            d->rightBinding->destroy(); d->rightBinding = 0;
        }
    }
    if (d->hCenterBinding) {
        targetPrivate->anchors()->resetHorizontalCenter();
        QDeclarativePropertyPrivate::setBinding(d->hCenterBinding->property(), 0);
        if (reason == ActualChange) {
            d->hCenterBinding->destroy(); d->hCenterBinding = 0;
        }
    }
    if (d->topBinding) {
        targetPrivate->anchors()->resetTop();
        QDeclarativePropertyPrivate::setBinding(d->topBinding->property(), 0);
        if (reason == ActualChange) {
            d->topBinding->destroy(); d->topBinding = 0;
        }
    }
    if (d->bottomBinding) {
        targetPrivate->anchors()->resetBottom();
        QDeclarativePropertyPrivate::setBinding(d->bottomBinding->property(), 0);
        if (reason == ActualChange) {
            d->bottomBinding->destroy(); d->bottomBinding = 0;
        }
    }
    if (d->vCenterBinding) {
        targetPrivate->anchors()->resetVerticalCenter();
        QDeclarativePropertyPrivate::setBinding(d->vCenterBinding->property(), 0);
        if (reason == ActualChange) {
            d->vCenterBinding->destroy(); d->vCenterBinding = 0;
        }
    }
    if (d->baselineBinding) {
        targetPrivate->anchors()->resetBaseline();
        QDeclarativePropertyPrivate::setBinding(d->baselineBinding->property(), 0);
        if (reason == ActualChange) {
            d->baselineBinding->destroy(); d->baselineBinding = 0;
        }
    }

    //restore previous anchors
    if (d->origLeftBinding)
        QDeclarativePropertyPrivate::setBinding(d->leftProp, d->origLeftBinding);
    if (d->origRightBinding)
        QDeclarativePropertyPrivate::setBinding(d->rightProp, d->origRightBinding);
    if (d->origHCenterBinding)
        QDeclarativePropertyPrivate::setBinding(d->hCenterProp, d->origHCenterBinding);
    if (d->origTopBinding)
        QDeclarativePropertyPrivate::setBinding(d->topProp, d->origTopBinding);
    if (d->origBottomBinding)
        QDeclarativePropertyPrivate::setBinding(d->bottomProp, d->origBottomBinding);
    if (d->origVCenterBinding)
        QDeclarativePropertyPrivate::setBinding(d->vCenterProp, d->origVCenterBinding);
    if (d->origBaselineBinding)
        QDeclarativePropertyPrivate::setBinding(d->baselineProp, d->origBaselineBinding);

    //restore any absolute geometry changed by the state's anchors
    QDeclarative1Anchors::Anchors stateVAnchors = d->anchorSet->d_func()->usedAnchors & QDeclarative1Anchors::Vertical_Mask;
    QDeclarative1Anchors::Anchors origVAnchors = targetPrivate->anchors()->usedAnchors() & QDeclarative1Anchors::Vertical_Mask;
    QDeclarative1Anchors::Anchors stateHAnchors = d->anchorSet->d_func()->usedAnchors & QDeclarative1Anchors::Horizontal_Mask;
    QDeclarative1Anchors::Anchors origHAnchors = targetPrivate->anchors()->usedAnchors() & QDeclarative1Anchors::Horizontal_Mask;

    bool stateSetWidth = (stateHAnchors &&
                          stateHAnchors != QDeclarative1Anchors::LeftAnchor &&
                          stateHAnchors != QDeclarative1Anchors::RightAnchor &&
                          stateHAnchors != QDeclarative1Anchors::HCenterAnchor);
    bool origSetWidth = (origHAnchors &&
                         origHAnchors != QDeclarative1Anchors::LeftAnchor &&
                         origHAnchors != QDeclarative1Anchors::RightAnchor &&
                         origHAnchors != QDeclarative1Anchors::HCenterAnchor);
    if (d->origWidth.isValid() && stateSetWidth && !origSetWidth)
        d->target->setWidth(d->origWidth.value);

    bool stateSetHeight = (stateVAnchors &&
                           stateVAnchors != QDeclarative1Anchors::TopAnchor &&
                           stateVAnchors != QDeclarative1Anchors::BottomAnchor &&
                           stateVAnchors != QDeclarative1Anchors::VCenterAnchor &&
                           stateVAnchors != QDeclarative1Anchors::BaselineAnchor);
    bool origSetHeight = (origVAnchors &&
                          origVAnchors != QDeclarative1Anchors::TopAnchor &&
                          origVAnchors != QDeclarative1Anchors::BottomAnchor &&
                          origVAnchors != QDeclarative1Anchors::VCenterAnchor &&
                          origVAnchors != QDeclarative1Anchors::BaselineAnchor);
    if (d->origHeight.isValid() && stateSetHeight && !origSetHeight)
        d->target->setHeight(d->origHeight.value);

    if (stateHAnchors && !origHAnchors)
        d->target->setX(d->origX);

    if (stateVAnchors && !origVAnchors)
        d->target->setY(d->origY);
}

QString QDeclarative1AnchorChanges::typeName() const
{
    return QLatin1String("AnchorChanges");
}

QList<QDeclarative1Action> QDeclarative1AnchorChanges::additionalActions()
{
    Q_D(QDeclarative1AnchorChanges);
    QList<QDeclarative1Action> extra;

    QDeclarative1Anchors::Anchors combined = d->anchorSet->d_func()->usedAnchors | d->anchorSet->d_func()->resetAnchors;
    bool hChange = combined & QDeclarative1Anchors::Horizontal_Mask;
    bool vChange = combined & QDeclarative1Anchors::Vertical_Mask;

    if (d->target) {
        QDeclarativeContext *ctxt = qmlContext(this);
        QDeclarative1Action a;
        if (hChange && d->fromX != d->toX) {
            a.property = QDeclarativeProperty(d->target, QLatin1String("x"), ctxt);
            a.toValue = d->toX;
            extra << a;
        }
        if (vChange && d->fromY != d->toY) {
            a.property = QDeclarativeProperty(d->target, QLatin1String("y"), ctxt);
            a.toValue = d->toY;
            extra << a;
        }
        if (hChange && d->fromWidth != d->toWidth) {
            a.property = QDeclarativeProperty(d->target, QLatin1String("width"), ctxt);
            a.toValue = d->toWidth;
            extra << a;
        }
        if (vChange && d->fromHeight != d->toHeight) {
            a.property = QDeclarativeProperty(d->target, QLatin1String("height"), ctxt);
            a.toValue = d->toHeight;
            extra << a;
        }
    }

    return extra;
}

bool QDeclarative1AnchorChanges::changesBindings()
{
    return true;
}

void QDeclarative1AnchorChanges::saveOriginals()
{
    Q_D(QDeclarative1AnchorChanges);
    if (!d->target)
        return;

    d->origLeftBinding = QDeclarativePropertyPrivate::binding(d->leftProp);
    d->origRightBinding = QDeclarativePropertyPrivate::binding(d->rightProp);
    d->origHCenterBinding = QDeclarativePropertyPrivate::binding(d->hCenterProp);
    d->origTopBinding = QDeclarativePropertyPrivate::binding(d->topProp);
    d->origBottomBinding = QDeclarativePropertyPrivate::binding(d->bottomProp);
    d->origVCenterBinding = QDeclarativePropertyPrivate::binding(d->vCenterProp);
    d->origBaselineBinding = QDeclarativePropertyPrivate::binding(d->baselineProp);

    QDeclarativeItemPrivate *targetPrivate = QDeclarativeItemPrivate::get(d->target);
    if (targetPrivate->widthValid)
        d->origWidth = d->target->width();
    if (targetPrivate->heightValid)
        d->origHeight = d->target->height();
    d->origX = d->target->x();
    d->origY = d->target->y();

    d->applyOrigLeft = d->applyOrigRight = d->applyOrigHCenter = d->applyOrigTop
      = d->applyOrigBottom = d->applyOrigVCenter = d->applyOrigBaseline = false;

    saveCurrentValues();
}

void QDeclarative1AnchorChanges::copyOriginals(QDeclarative1ActionEvent *other)
{
    Q_D(QDeclarative1AnchorChanges);
    QDeclarative1AnchorChanges *ac = static_cast<QDeclarative1AnchorChanges*>(other);
    QDeclarative1AnchorChangesPrivate *acp = ac->d_func();

    QDeclarative1Anchors::Anchors combined = acp->anchorSet->d_func()->usedAnchors |
                                            acp->anchorSet->d_func()->resetAnchors;

    //probably also need to revert some things
    d->applyOrigLeft = (combined & QDeclarative1Anchors::LeftAnchor);
    d->applyOrigRight = (combined & QDeclarative1Anchors::RightAnchor);
    d->applyOrigHCenter = (combined & QDeclarative1Anchors::HCenterAnchor);
    d->applyOrigTop = (combined & QDeclarative1Anchors::TopAnchor);
    d->applyOrigBottom = (combined & QDeclarative1Anchors::BottomAnchor);
    d->applyOrigVCenter = (combined & QDeclarative1Anchors::VCenterAnchor);
    d->applyOrigBaseline = (combined & QDeclarative1Anchors::BaselineAnchor);

    d->origLeftBinding = acp->origLeftBinding;
    d->origRightBinding = acp->origRightBinding;
    d->origHCenterBinding = acp->origHCenterBinding;
    d->origTopBinding = acp->origTopBinding;
    d->origBottomBinding = acp->origBottomBinding;
    d->origVCenterBinding = acp->origVCenterBinding;
    d->origBaselineBinding = acp->origBaselineBinding;

    d->origWidth = acp->origWidth;
    d->origHeight = acp->origHeight;
    d->origX = acp->origX;
    d->origY = acp->origY;

    d->oldBindings.clear();
    d->oldBindings << acp->leftBinding << acp->rightBinding << acp->hCenterBinding
                << acp->topBinding << acp->bottomBinding << acp->baselineBinding;

    saveCurrentValues();
}

void QDeclarative1AnchorChanges::clearBindings()
{
    Q_D(QDeclarative1AnchorChanges);
    if (!d->target)
        return;

    //### should this (saving "from" values) be moved to saveCurrentValues()?
    d->fromX = d->target->x();
    d->fromY = d->target->y();
    d->fromWidth = d->target->width();
    d->fromHeight = d->target->height();

    QDeclarativeItemPrivate *targetPrivate = QDeclarativeItemPrivate::get(d->target);
    //reset any anchors with corresponding reverts
    //reset any anchors that have been specified as "undefined"
    //reset any anchors that we'll be setting in the state
    QDeclarative1Anchors::Anchors combined = d->anchorSet->d_func()->resetAnchors |
                                            d->anchorSet->d_func()->usedAnchors;
    if (d->applyOrigLeft || (combined & QDeclarative1Anchors::LeftAnchor)) {
        targetPrivate->anchors()->resetLeft();
        QDeclarativePropertyPrivate::setBinding(d->leftProp, 0);
    }
    if (d->applyOrigRight || (combined & QDeclarative1Anchors::RightAnchor)) {
        targetPrivate->anchors()->resetRight();
        QDeclarativePropertyPrivate::setBinding(d->rightProp, 0);
    }
    if (d->applyOrigHCenter || (combined & QDeclarative1Anchors::HCenterAnchor)) {
        targetPrivate->anchors()->resetHorizontalCenter();
        QDeclarativePropertyPrivate::setBinding(d->hCenterProp, 0);
    }
    if (d->applyOrigTop || (combined & QDeclarative1Anchors::TopAnchor)) {
        targetPrivate->anchors()->resetTop();
        QDeclarativePropertyPrivate::setBinding(d->topProp, 0);
    }
    if (d->applyOrigBottom || (combined & QDeclarative1Anchors::BottomAnchor)) {
        targetPrivate->anchors()->resetBottom();
        QDeclarativePropertyPrivate::setBinding(d->bottomProp, 0);
    }
    if (d->applyOrigVCenter || (combined & QDeclarative1Anchors::VCenterAnchor)) {
        targetPrivate->anchors()->resetVerticalCenter();
        QDeclarativePropertyPrivate::setBinding(d->vCenterProp, 0);
    }
    if (d->applyOrigBaseline || (combined & QDeclarative1Anchors::BaselineAnchor)) {
        targetPrivate->anchors()->resetBaseline();
        QDeclarativePropertyPrivate::setBinding(d->baselineProp, 0);
    }
}

bool QDeclarative1AnchorChanges::override(QDeclarative1ActionEvent*other)
{
    if (other->typeName() != QLatin1String("AnchorChanges"))
        return false;
    if (static_cast<QDeclarative1ActionEvent*>(this) == other)
        return true;
    if (static_cast<QDeclarative1AnchorChanges*>(other)->object() == object())
        return true;
    return false;
}

void QDeclarative1AnchorChanges::rewind()
{
    Q_D(QDeclarative1AnchorChanges);
    if (!d->target)
        return;

    QDeclarativeItemPrivate *targetPrivate = QDeclarativeItemPrivate::get(d->target);

    //restore previous values (but not previous bindings, i.e. anchors)
    d->target->setX(d->rewindX);
    d->target->setY(d->rewindY);
    if (targetPrivate->widthValid) {
        d->target->setWidth(d->rewindWidth);
    }
    if (targetPrivate->heightValid) {
        d->target->setHeight(d->rewindHeight);
    }
}

void QDeclarative1AnchorChanges::saveCurrentValues()
{
    Q_D(QDeclarative1AnchorChanges);
    if (!d->target)
        return;

    QDeclarativeItemPrivate *targetPrivate = QDeclarativeItemPrivate::get(d->target);
    d->rewindLeft = targetPrivate->anchors()->left();
    d->rewindRight = targetPrivate->anchors()->right();
    d->rewindHCenter = targetPrivate->anchors()->horizontalCenter();
    d->rewindTop = targetPrivate->anchors()->top();
    d->rewindBottom = targetPrivate->anchors()->bottom();
    d->rewindVCenter = targetPrivate->anchors()->verticalCenter();
    d->rewindBaseline = targetPrivate->anchors()->baseline();

    d->rewindX = d->target->x();
    d->rewindY = d->target->y();
    d->rewindWidth = d->target->width();
    d->rewindHeight = d->target->height();
}

void QDeclarative1AnchorChanges::saveTargetValues()
{
    Q_D(QDeclarative1AnchorChanges);
    if (!d->target)
        return;

    d->toX = d->target->x();
    d->toY = d->target->y();
    d->toWidth = d->target->width();
    d->toHeight = d->target->height();
}

#include <qdeclarativestateoperations.moc>
#include <moc_qdeclarativestateoperations_p.cpp>



QT_END_NAMESPACE

