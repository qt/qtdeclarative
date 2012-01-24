/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
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
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qquickstateoperations_p.h"
#include "qquickitem_p.h"

#include <private/qdeclarativestate_p_p.h>

#include <QtDeclarative/qdeclarativeinfo.h>
#include <QtCore/qmath.h>

QT_BEGIN_NAMESPACE

class QQuickParentChangePrivate : public QDeclarativeStateOperationPrivate
{
    Q_DECLARE_PUBLIC(QQuickParentChange)
public:
    QQuickParentChangePrivate() : target(0), parent(0), origParent(0), origStackBefore(0),
        rewindParent(0), rewindStackBefore(0) {}

    QQuickItem *target;
    QDeclarativeGuard<QQuickItem> parent;
    QDeclarativeGuard<QQuickItem> origParent;
    QDeclarativeGuard<QQuickItem> origStackBefore;
    QQuickItem *rewindParent;
    QQuickItem *rewindStackBefore;

    QDeclarativeNullableValue<QDeclarativeScriptString> xString;
    QDeclarativeNullableValue<QDeclarativeScriptString> yString;
    QDeclarativeNullableValue<QDeclarativeScriptString> widthString;
    QDeclarativeNullableValue<QDeclarativeScriptString> heightString;
    QDeclarativeNullableValue<QDeclarativeScriptString> scaleString;
    QDeclarativeNullableValue<QDeclarativeScriptString> rotationString;

    void doChange(QQuickItem *targetParent, QQuickItem *stackBefore = 0);
};

void QQuickParentChangePrivate::doChange(QQuickItem *targetParent, QQuickItem *stackBefore)
{
    if (targetParent && target && target->parentItem()) {
        Q_Q(QQuickParentChange);
        bool ok;
        const QTransform &transform = target->parentItem()->itemTransform(targetParent, &ok);
        if (transform.type() >= QTransform::TxShear || !ok) {
            qmlInfo(q) << QQuickParentChange::tr("Unable to preserve appearance under complex transform");
            ok = false;
        }

        qreal scale = 1;
        qreal rotation = 0;
        bool isRotate = (transform.type() == QTransform::TxRotate) || (transform.m11() < 0);
        if (ok && !isRotate) {
            if (transform.m11() == transform.m22())
                scale = transform.m11();
            else {
                qmlInfo(q) << QQuickParentChange::tr("Unable to preserve appearance under non-uniform scale");
                ok = false;
            }
        } else if (ok && isRotate) {
            if (transform.m11() == transform.m22())
                scale = qSqrt(transform.m11()*transform.m11() + transform.m12()*transform.m12());
            else {
                qmlInfo(q) << QQuickParentChange::tr("Unable to preserve appearance under non-uniform scale");
                ok = false;
            }

            if (scale != 0)
                rotation = atan2(transform.m12()/scale, transform.m11()/scale) * 180/M_PI;
            else {
                qmlInfo(q) << QQuickParentChange::tr("Unable to preserve appearance under scale of 0");
                ok = false;
            }
        }

        const QPointF &point = transform.map(QPointF(target->x(),target->y()));
        qreal x = point.x();
        qreal y = point.y();

        // setParentItem will update the transformOriginPoint if needed
        target->setParentItem(targetParent);

        if (ok && target->transformOrigin() != QQuickItem::TopLeft) {
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

QQuickParentChange::QQuickParentChange(QObject *parent)
    : QDeclarativeStateOperation(*(new QQuickParentChangePrivate), parent)
{
}

QQuickParentChange::~QQuickParentChange()
{
}

QDeclarativeScriptString QQuickParentChange::x() const
{
    Q_D(const QQuickParentChange);
    return d->xString.value;
}

void QQuickParentChange::setX(QDeclarativeScriptString x)
{
    Q_D(QQuickParentChange);
    d->xString = x;
}

bool QQuickParentChange::xIsSet() const
{
    Q_D(const QQuickParentChange);
    return d->xString.isValid();
}

QDeclarativeScriptString QQuickParentChange::y() const
{
    Q_D(const QQuickParentChange);
    return d->yString.value;
}

void QQuickParentChange::setY(QDeclarativeScriptString y)
{
    Q_D(QQuickParentChange);
    d->yString = y;
}

bool QQuickParentChange::yIsSet() const
{
    Q_D(const QQuickParentChange);
    return d->yString.isValid();
}

QDeclarativeScriptString QQuickParentChange::width() const
{
    Q_D(const QQuickParentChange);
    return d->widthString.value;
}

void QQuickParentChange::setWidth(QDeclarativeScriptString width)
{
    Q_D(QQuickParentChange);
    d->widthString = width;
}

bool QQuickParentChange::widthIsSet() const
{
    Q_D(const QQuickParentChange);
    return d->widthString.isValid();
}

QDeclarativeScriptString QQuickParentChange::height() const
{
    Q_D(const QQuickParentChange);
    return d->heightString.value;
}

void QQuickParentChange::setHeight(QDeclarativeScriptString height)
{
    Q_D(QQuickParentChange);
    d->heightString = height;
}

bool QQuickParentChange::heightIsSet() const
{
    Q_D(const QQuickParentChange);
    return d->heightString.isValid();
}

QDeclarativeScriptString QQuickParentChange::scale() const
{
    Q_D(const QQuickParentChange);
    return d->scaleString.value;
}

void QQuickParentChange::setScale(QDeclarativeScriptString scale)
{
    Q_D(QQuickParentChange);
    d->scaleString = scale;
}

bool QQuickParentChange::scaleIsSet() const
{
    Q_D(const QQuickParentChange);
    return d->scaleString.isValid();
}

QDeclarativeScriptString QQuickParentChange::rotation() const
{
    Q_D(const QQuickParentChange);
    return d->rotationString.value;
}

void QQuickParentChange::setRotation(QDeclarativeScriptString rotation)
{
    Q_D(QQuickParentChange);
    d->rotationString = rotation;
}

bool QQuickParentChange::rotationIsSet() const
{
    Q_D(const QQuickParentChange);
    return d->rotationString.isValid();
}

QQuickItem *QQuickParentChange::originalParent() const
{
    Q_D(const QQuickParentChange);
    return d->origParent;
}

QQuickItem *QQuickParentChange::object() const
{
    Q_D(const QQuickParentChange);
    return d->target;
}

void QQuickParentChange::setObject(QQuickItem *target)
{
    Q_D(QQuickParentChange);
    d->target = target;
}

QQuickItem *QQuickParentChange::parent() const
{
    Q_D(const QQuickParentChange);
    return d->parent;
}

void QQuickParentChange::setParent(QQuickItem *parent)
{
    Q_D(QQuickParentChange);
    d->parent = parent;
}

QDeclarativeStateOperation::ActionList QQuickParentChange::actions()
{
    Q_D(QQuickParentChange);
    if (!d->target || !d->parent)
        return ActionList();

    ActionList actions;

    QDeclarativeAction a;
    a.event = this;
    actions << a;

    if (d->xString.isValid()) {
        bool ok = false;
        QString script = d->xString.value.script();
        qreal x = script.toFloat(&ok);
        if (ok) {
            QDeclarativeAction xa(d->target, QLatin1String("x"), x);
            actions << xa;
        } else {
            QDeclarativeBinding *newBinding = new QDeclarativeBinding(script, d->target, qmlContext(this));
            QDeclarativeProperty property(d->target, QLatin1String("x"));
            newBinding->setTarget(property);
            QDeclarativeAction xa;
            xa.property = property;
            xa.toBinding = newBinding;
            xa.fromValue = xa.property.read();
            xa.deletableToBinding = true;
            actions << xa;
        }
    }

    if (d->yString.isValid()) {
        bool ok = false;
        QString script = d->yString.value.script();
        qreal y = script.toFloat(&ok);
        if (ok) {
            QDeclarativeAction ya(d->target, QLatin1String("y"), y);
            actions << ya;
        } else {
            QDeclarativeBinding *newBinding = new QDeclarativeBinding(script, d->target, qmlContext(this));
            QDeclarativeProperty property(d->target, QLatin1String("y"));
            newBinding->setTarget(property);
            QDeclarativeAction ya;
            ya.property = property;
            ya.toBinding = newBinding;
            ya.fromValue = ya.property.read();
            ya.deletableToBinding = true;
            actions << ya;
        }
    }

    if (d->scaleString.isValid()) {
        bool ok = false;
        QString script = d->scaleString.value.script();
        qreal scale = script.toFloat(&ok);
        if (ok) {
            QDeclarativeAction sa(d->target, QLatin1String("scale"), scale);
            actions << sa;
        } else {
            QDeclarativeBinding *newBinding = new QDeclarativeBinding(script, d->target, qmlContext(this));
            QDeclarativeProperty property(d->target, QLatin1String("scale"));
            newBinding->setTarget(property);
            QDeclarativeAction sa;
            sa.property = property;
            sa.toBinding = newBinding;
            sa.fromValue = sa.property.read();
            sa.deletableToBinding = true;
            actions << sa;
        }
    }

    if (d->rotationString.isValid()) {
        bool ok = false;
        QString script = d->rotationString.value.script();
        qreal rotation = script.toFloat(&ok);
        if (ok) {
            QDeclarativeAction ra(d->target, QLatin1String("rotation"), rotation);
            actions << ra;
        } else {
            QDeclarativeBinding *newBinding = new QDeclarativeBinding(script, d->target, qmlContext(this));
            QDeclarativeProperty property(d->target, QLatin1String("rotation"));
            newBinding->setTarget(property);
            QDeclarativeAction ra;
            ra.property = property;
            ra.toBinding = newBinding;
            ra.fromValue = ra.property.read();
            ra.deletableToBinding = true;
            actions << ra;
        }
    }

    if (d->widthString.isValid()) {
        bool ok = false;
        QString script = d->widthString.value.script();
        qreal width = script.toFloat(&ok);
        if (ok) {
            QDeclarativeAction wa(d->target, QLatin1String("width"), width);
            actions << wa;
        } else {
            QDeclarativeBinding *newBinding = new QDeclarativeBinding(script, d->target, qmlContext(this));
            QDeclarativeProperty property(d->target, QLatin1String("width"));
            newBinding->setTarget(property);
            QDeclarativeAction wa;
            wa.property = property;
            wa.toBinding = newBinding;
            wa.fromValue = wa.property.read();
            wa.deletableToBinding = true;
            actions << wa;
        }
    }

    if (d->heightString.isValid()) {
        bool ok = false;
        QString script = d->heightString.value.script();
        qreal height = script.toFloat(&ok);
        if (ok) {
            QDeclarativeAction ha(d->target, QLatin1String("height"), height);
            actions << ha;
        } else {
            QDeclarativeBinding *newBinding = new QDeclarativeBinding(script, d->target, qmlContext(this));
            QDeclarativeProperty property(d->target, QLatin1String("height"));
            newBinding->setTarget(property);
            QDeclarativeAction ha;
            ha.property = property;
            ha.toBinding = newBinding;
            ha.fromValue = ha.property.read();
            ha.deletableToBinding = true;
            actions << ha;
        }
    }

    return actions;
}

void QQuickParentChange::saveOriginals()
{
    Q_D(QQuickParentChange);
    saveCurrentValues();
    d->origParent = d->rewindParent;
    d->origStackBefore = d->rewindStackBefore;
}

/*void QQuickParentChange::copyOriginals(QDeclarativeActionEvent *other)
{
    Q_D(QQuickParentChange);
    QQuickParentChange *pc = static_cast<QQuickParentChange*>(other);

    d->origParent = pc->d_func()->rewindParent;
    d->origStackBefore = pc->d_func()->rewindStackBefore;

    saveCurrentValues();
}*/

void QQuickParentChange::execute(Reason)
{
    Q_D(QQuickParentChange);
    d->doChange(d->parent);
}

bool QQuickParentChange::isReversable()
{
    return true;
}

void QQuickParentChange::reverse(Reason)
{
    Q_D(QQuickParentChange);
    d->doChange(d->origParent, d->origStackBefore);
}

QString QQuickParentChange::typeName() const
{
    return QLatin1String("ParentChange");
}

bool QQuickParentChange::override(QDeclarativeActionEvent*other)
{
    Q_D(QQuickParentChange);
    if (other->typeName() != QLatin1String("ParentChange"))
        return false;
    if (QQuickParentChange *otherPC = static_cast<QQuickParentChange*>(other))
        return (d->target == otherPC->object());
    return false;
}

void QQuickParentChange::saveCurrentValues()
{
    Q_D(QQuickParentChange);
    if (!d->target) {
        d->rewindParent = 0;
        d->rewindStackBefore = 0;
        return;
    }

    d->rewindParent = d->target->parentItem();
    d->rewindStackBefore = 0;

    if (!d->rewindParent)
        return;

    QList<QQuickItem *> children = d->rewindParent->childItems();
    for (int ii = 0; ii < children.count() - 1; ++ii) {
        if (children.at(ii) == d->target) {
            d->rewindStackBefore = children.at(ii + 1);
            break;
        }
    }
}

void QQuickParentChange::rewind()
{
    Q_D(QQuickParentChange);
    d->doChange(d->rewindParent, d->rewindStackBefore);
}

class QQuickAnchorSetPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QQuickAnchorSet)
public:
    QQuickAnchorSetPrivate()
      : usedAnchors(0), resetAnchors(0), fill(0),
        centerIn(0)/*, leftMargin(0), rightMargin(0), topMargin(0), bottomMargin(0),
        margins(0), vCenterOffset(0), hCenterOffset(0), baselineOffset(0)*/
    {
    }

    QQuickAnchors::Anchors usedAnchors;
    QQuickAnchors::Anchors resetAnchors;

    QQuickItem *fill;
    QQuickItem *centerIn;

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

QQuickAnchorSet::QQuickAnchorSet(QObject *parent)
  : QObject(*new QQuickAnchorSetPrivate, parent)
{
}

QQuickAnchorSet::~QQuickAnchorSet()
{
}

QDeclarativeScriptString QQuickAnchorSet::top() const
{
    Q_D(const QQuickAnchorSet);
    return d->topScript;
}

void QQuickAnchorSet::setTop(const QDeclarativeScriptString &edge)
{
    Q_D(QQuickAnchorSet);
    d->usedAnchors |= QQuickAnchors::TopAnchor;
    d->topScript = edge;
    if (edge.script() == QLatin1String("undefined"))
        resetTop();
}

void QQuickAnchorSet::resetTop()
{
    Q_D(QQuickAnchorSet);
    d->usedAnchors &= ~QQuickAnchors::TopAnchor;
    d->topScript = QDeclarativeScriptString();
    d->resetAnchors |= QQuickAnchors::TopAnchor;
}

QDeclarativeScriptString QQuickAnchorSet::bottom() const
{
    Q_D(const QQuickAnchorSet);
    return d->bottomScript;
}

void QQuickAnchorSet::setBottom(const QDeclarativeScriptString &edge)
{
    Q_D(QQuickAnchorSet);
    d->usedAnchors |= QQuickAnchors::BottomAnchor;
    d->bottomScript = edge;
    if (edge.script() == QLatin1String("undefined"))
        resetBottom();
}

void QQuickAnchorSet::resetBottom()
{
    Q_D(QQuickAnchorSet);
    d->usedAnchors &= ~QQuickAnchors::BottomAnchor;
    d->bottomScript = QDeclarativeScriptString();
    d->resetAnchors |= QQuickAnchors::BottomAnchor;
}

QDeclarativeScriptString QQuickAnchorSet::verticalCenter() const
{
    Q_D(const QQuickAnchorSet);
    return d->vCenterScript;
}

void QQuickAnchorSet::setVerticalCenter(const QDeclarativeScriptString &edge)
{
    Q_D(QQuickAnchorSet);
    d->usedAnchors |= QQuickAnchors::VCenterAnchor;
    d->vCenterScript = edge;
    if (edge.script() == QLatin1String("undefined"))
        resetVerticalCenter();
}

void QQuickAnchorSet::resetVerticalCenter()
{
    Q_D(QQuickAnchorSet);
    d->usedAnchors &= ~QQuickAnchors::VCenterAnchor;
    d->vCenterScript = QDeclarativeScriptString();
    d->resetAnchors |= QQuickAnchors::VCenterAnchor;
}

QDeclarativeScriptString QQuickAnchorSet::baseline() const
{
    Q_D(const QQuickAnchorSet);
    return d->baselineScript;
}

void QQuickAnchorSet::setBaseline(const QDeclarativeScriptString &edge)
{
    Q_D(QQuickAnchorSet);
    d->usedAnchors |= QQuickAnchors::BaselineAnchor;
    d->baselineScript = edge;
    if (edge.script() == QLatin1String("undefined"))
        resetBaseline();
}

void QQuickAnchorSet::resetBaseline()
{
    Q_D(QQuickAnchorSet);
    d->usedAnchors &= ~QQuickAnchors::BaselineAnchor;
    d->baselineScript = QDeclarativeScriptString();
    d->resetAnchors |= QQuickAnchors::BaselineAnchor;
}

QDeclarativeScriptString QQuickAnchorSet::left() const
{
    Q_D(const QQuickAnchorSet);
    return d->leftScript;
}

void QQuickAnchorSet::setLeft(const QDeclarativeScriptString &edge)
{
    Q_D(QQuickAnchorSet);
    d->usedAnchors |= QQuickAnchors::LeftAnchor;
    d->leftScript = edge;
    if (edge.script() == QLatin1String("undefined"))
        resetLeft();
}

void QQuickAnchorSet::resetLeft()
{
    Q_D(QQuickAnchorSet);
    d->usedAnchors &= ~QQuickAnchors::LeftAnchor;
    d->leftScript = QDeclarativeScriptString();
    d->resetAnchors |= QQuickAnchors::LeftAnchor;
}

QDeclarativeScriptString QQuickAnchorSet::right() const
{
    Q_D(const QQuickAnchorSet);
    return d->rightScript;
}

void QQuickAnchorSet::setRight(const QDeclarativeScriptString &edge)
{
    Q_D(QQuickAnchorSet);
    d->usedAnchors |= QQuickAnchors::RightAnchor;
    d->rightScript = edge;
    if (edge.script() == QLatin1String("undefined"))
        resetRight();
}

void QQuickAnchorSet::resetRight()
{
    Q_D(QQuickAnchorSet);
    d->usedAnchors &= ~QQuickAnchors::RightAnchor;
    d->rightScript = QDeclarativeScriptString();
    d->resetAnchors |= QQuickAnchors::RightAnchor;
}

QDeclarativeScriptString QQuickAnchorSet::horizontalCenter() const
{
    Q_D(const QQuickAnchorSet);
    return d->hCenterScript;
}

void QQuickAnchorSet::setHorizontalCenter(const QDeclarativeScriptString &edge)
{
    Q_D(QQuickAnchorSet);
    d->usedAnchors |= QQuickAnchors::HCenterAnchor;
    d->hCenterScript = edge;
    if (edge.script() == QLatin1String("undefined"))
        resetHorizontalCenter();
}

void QQuickAnchorSet::resetHorizontalCenter()
{
    Q_D(QQuickAnchorSet);
    d->usedAnchors &= ~QQuickAnchors::HCenterAnchor;
    d->hCenterScript = QDeclarativeScriptString();
    d->resetAnchors |= QQuickAnchors::HCenterAnchor;
}

QQuickItem *QQuickAnchorSet::fill() const
{
    Q_D(const QQuickAnchorSet);
    return d->fill;
}

void QQuickAnchorSet::setFill(QQuickItem *f)
{
    Q_D(QQuickAnchorSet);
    d->fill = f;
}

void QQuickAnchorSet::resetFill()
{
    setFill(0);
}

QQuickItem *QQuickAnchorSet::centerIn() const
{
    Q_D(const QQuickAnchorSet);
    return d->centerIn;
}

void QQuickAnchorSet::setCenterIn(QQuickItem* c)
{
    Q_D(QQuickAnchorSet);
    d->centerIn = c;
}

void QQuickAnchorSet::resetCenterIn()
{
    setCenterIn(0);
}


class QQuickAnchorChangesPrivate : public QDeclarativeStateOperationPrivate
{
public:
    QQuickAnchorChangesPrivate()
        : target(0), anchorSet(new QQuickAnchorSet),
          leftBinding(0), rightBinding(0), hCenterBinding(0),
          topBinding(0), bottomBinding(0), vCenterBinding(0), baselineBinding(0),
          origLeftBinding(0), origRightBinding(0), origHCenterBinding(0),
          origTopBinding(0), origBottomBinding(0), origVCenterBinding(0),
          origBaselineBinding(0)
    {

    }
    ~QQuickAnchorChangesPrivate() { delete anchorSet; }

    QQuickItem *target;
    QQuickAnchorSet *anchorSet;

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

    QQuickAnchorLine rewindLeft;
    QQuickAnchorLine rewindRight;
    QQuickAnchorLine rewindHCenter;
    QQuickAnchorLine rewindTop;
    QQuickAnchorLine rewindBottom;
    QQuickAnchorLine rewindVCenter;
    QQuickAnchorLine rewindBaseline;

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

QQuickAnchorChanges::QQuickAnchorChanges(QObject *parent)
 : QDeclarativeStateOperation(*(new QQuickAnchorChangesPrivate), parent)
{
}

QQuickAnchorChanges::~QQuickAnchorChanges()
{
}

QQuickAnchorChanges::ActionList QQuickAnchorChanges::actions()
{
    Q_D(QQuickAnchorChanges);
    d->leftBinding = d->rightBinding = d->hCenterBinding = d->topBinding
                   = d->bottomBinding = d->vCenterBinding = d->baselineBinding = 0;

    d->leftProp = QDeclarativeProperty(d->target, QLatin1String("anchors.left"));
    d->rightProp = QDeclarativeProperty(d->target, QLatin1String("anchors.right"));
    d->hCenterProp = QDeclarativeProperty(d->target, QLatin1String("anchors.horizontalCenter"));
    d->topProp = QDeclarativeProperty(d->target, QLatin1String("anchors.top"));
    d->bottomProp = QDeclarativeProperty(d->target, QLatin1String("anchors.bottom"));
    d->vCenterProp = QDeclarativeProperty(d->target, QLatin1String("anchors.verticalCenter"));
    d->baselineProp = QDeclarativeProperty(d->target, QLatin1String("anchors.baseline"));

    if (d->anchorSet->d_func()->usedAnchors & QQuickAnchors::LeftAnchor) {
        d->leftBinding = new QDeclarativeBinding(d->anchorSet->d_func()->leftScript.script(), d->target, qmlContext(this));
        d->leftBinding->setTarget(d->leftProp);
    }
    if (d->anchorSet->d_func()->usedAnchors & QQuickAnchors::RightAnchor) {
        d->rightBinding = new QDeclarativeBinding(d->anchorSet->d_func()->rightScript.script(), d->target, qmlContext(this));
        d->rightBinding->setTarget(d->rightProp);
    }
    if (d->anchorSet->d_func()->usedAnchors & QQuickAnchors::HCenterAnchor) {
        d->hCenterBinding = new QDeclarativeBinding(d->anchorSet->d_func()->hCenterScript.script(), d->target, qmlContext(this));
        d->hCenterBinding->setTarget(d->hCenterProp);
    }
    if (d->anchorSet->d_func()->usedAnchors & QQuickAnchors::TopAnchor) {
        d->topBinding = new QDeclarativeBinding(d->anchorSet->d_func()->topScript.script(), d->target, qmlContext(this));
        d->topBinding->setTarget(d->topProp);
    }
    if (d->anchorSet->d_func()->usedAnchors & QQuickAnchors::BottomAnchor) {
        d->bottomBinding = new QDeclarativeBinding(d->anchorSet->d_func()->bottomScript.script(), d->target, qmlContext(this));
        d->bottomBinding->setTarget(d->bottomProp);
    }
    if (d->anchorSet->d_func()->usedAnchors & QQuickAnchors::VCenterAnchor) {
        d->vCenterBinding = new QDeclarativeBinding(d->anchorSet->d_func()->vCenterScript.script(), d->target, qmlContext(this));
        d->vCenterBinding->setTarget(d->vCenterProp);
    }
    if (d->anchorSet->d_func()->usedAnchors & QQuickAnchors::BaselineAnchor) {
        d->baselineBinding = new QDeclarativeBinding(d->anchorSet->d_func()->baselineScript.script(), d->target, qmlContext(this));
        d->baselineBinding->setTarget(d->baselineProp);
    }

    QDeclarativeAction a;
    a.event = this;
    return ActionList() << a;
}

QQuickAnchorSet *QQuickAnchorChanges::anchors()
{
    Q_D(QQuickAnchorChanges);
    return d->anchorSet;
}

QQuickItem *QQuickAnchorChanges::object() const
{
    Q_D(const QQuickAnchorChanges);
    return d->target;
}

void QQuickAnchorChanges::setObject(QQuickItem *target)
{
    Q_D(QQuickAnchorChanges);
    d->target = target;
}

void QQuickAnchorChanges::execute(Reason reason)
{
    Q_D(QQuickAnchorChanges);
    if (!d->target)
        return;

    QQuickItemPrivate *targetPrivate = QQuickItemPrivate::get(d->target);
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
    if (d->anchorSet->d_func()->resetAnchors & QQuickAnchors::LeftAnchor) {
        targetPrivate->anchors()->resetLeft();
        QDeclarativePropertyPrivate::setBinding(d->leftProp, 0);
    }
    if (d->anchorSet->d_func()->resetAnchors & QQuickAnchors::RightAnchor) {
        targetPrivate->anchors()->resetRight();
        QDeclarativePropertyPrivate::setBinding(d->rightProp, 0);
    }
    if (d->anchorSet->d_func()->resetAnchors & QQuickAnchors::HCenterAnchor) {
        targetPrivate->anchors()->resetHorizontalCenter();
        QDeclarativePropertyPrivate::setBinding(d->hCenterProp, 0);
    }
    if (d->anchorSet->d_func()->resetAnchors & QQuickAnchors::TopAnchor) {
        targetPrivate->anchors()->resetTop();
        QDeclarativePropertyPrivate::setBinding(d->topProp, 0);
    }
    if (d->anchorSet->d_func()->resetAnchors & QQuickAnchors::BottomAnchor) {
        targetPrivate->anchors()->resetBottom();
        QDeclarativePropertyPrivate::setBinding(d->bottomProp, 0);
    }
    if (d->anchorSet->d_func()->resetAnchors & QQuickAnchors::VCenterAnchor) {
        targetPrivate->anchors()->resetVerticalCenter();
        QDeclarativePropertyPrivate::setBinding(d->vCenterProp, 0);
    }
    if (d->anchorSet->d_func()->resetAnchors & QQuickAnchors::BaselineAnchor) {
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

bool QQuickAnchorChanges::isReversable()
{
    return true;
}

void QQuickAnchorChanges::reverse(Reason reason)
{
    Q_D(QQuickAnchorChanges);
    if (!d->target)
        return;

    QQuickItemPrivate *targetPrivate = QQuickItemPrivate::get(d->target);
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
    QQuickAnchors::Anchors stateVAnchors = d->anchorSet->d_func()->usedAnchors & QQuickAnchors::Vertical_Mask;
    QQuickAnchors::Anchors origVAnchors = targetPrivate->anchors()->usedAnchors() & QQuickAnchors::Vertical_Mask;
    QQuickAnchors::Anchors stateHAnchors = d->anchorSet->d_func()->usedAnchors & QQuickAnchors::Horizontal_Mask;
    QQuickAnchors::Anchors origHAnchors = targetPrivate->anchors()->usedAnchors() & QQuickAnchors::Horizontal_Mask;

    bool stateSetWidth = (stateHAnchors &&
                          stateHAnchors != QQuickAnchors::LeftAnchor &&
                          stateHAnchors != QQuickAnchors::RightAnchor &&
                          stateHAnchors != QQuickAnchors::HCenterAnchor);
    bool origSetWidth = (origHAnchors &&
                         origHAnchors != QQuickAnchors::LeftAnchor &&
                         origHAnchors != QQuickAnchors::RightAnchor &&
                         origHAnchors != QQuickAnchors::HCenterAnchor);
    if (d->origWidth.isValid() && stateSetWidth && !origSetWidth)
        d->target->setWidth(d->origWidth.value);

    bool stateSetHeight = (stateVAnchors &&
                           stateVAnchors != QQuickAnchors::TopAnchor &&
                           stateVAnchors != QQuickAnchors::BottomAnchor &&
                           stateVAnchors != QQuickAnchors::VCenterAnchor &&
                           stateVAnchors != QQuickAnchors::BaselineAnchor);
    bool origSetHeight = (origVAnchors &&
                          origVAnchors != QQuickAnchors::TopAnchor &&
                          origVAnchors != QQuickAnchors::BottomAnchor &&
                          origVAnchors != QQuickAnchors::VCenterAnchor &&
                          origVAnchors != QQuickAnchors::BaselineAnchor);
    if (d->origHeight.isValid() && stateSetHeight && !origSetHeight)
        d->target->setHeight(d->origHeight.value);

    if (stateHAnchors && !origHAnchors)
        d->target->setX(d->origX);

    if (stateVAnchors && !origVAnchors)
        d->target->setY(d->origY);
}

QString QQuickAnchorChanges::typeName() const
{
    return QLatin1String("AnchorChanges");
}

QList<QDeclarativeAction> QQuickAnchorChanges::additionalActions()
{
    Q_D(QQuickAnchorChanges);
    QList<QDeclarativeAction> extra;

    QQuickAnchors::Anchors combined = d->anchorSet->d_func()->usedAnchors | d->anchorSet->d_func()->resetAnchors;
    bool hChange = combined & QQuickAnchors::Horizontal_Mask;
    bool vChange = combined & QQuickAnchors::Vertical_Mask;

    if (d->target) {
        QDeclarativeAction a;
        if (hChange && d->fromX != d->toX) {
            a.property = QDeclarativeProperty(d->target, QLatin1String("x"));
            a.toValue = d->toX;
            extra << a;
        }
        if (vChange && d->fromY != d->toY) {
            a.property = QDeclarativeProperty(d->target, QLatin1String("y"));
            a.toValue = d->toY;
            extra << a;
        }
        if (hChange && d->fromWidth != d->toWidth) {
            a.property = QDeclarativeProperty(d->target, QLatin1String("width"));
            a.toValue = d->toWidth;
            extra << a;
        }
        if (vChange && d->fromHeight != d->toHeight) {
            a.property = QDeclarativeProperty(d->target, QLatin1String("height"));
            a.toValue = d->toHeight;
            extra << a;
        }
    }

    return extra;
}

bool QQuickAnchorChanges::changesBindings()
{
    return true;
}

void QQuickAnchorChanges::saveOriginals()
{
    Q_D(QQuickAnchorChanges);
    if (!d->target)
        return;

    d->origLeftBinding = QDeclarativePropertyPrivate::binding(d->leftProp);
    d->origRightBinding = QDeclarativePropertyPrivate::binding(d->rightProp);
    d->origHCenterBinding = QDeclarativePropertyPrivate::binding(d->hCenterProp);
    d->origTopBinding = QDeclarativePropertyPrivate::binding(d->topProp);
    d->origBottomBinding = QDeclarativePropertyPrivate::binding(d->bottomProp);
    d->origVCenterBinding = QDeclarativePropertyPrivate::binding(d->vCenterProp);
    d->origBaselineBinding = QDeclarativePropertyPrivate::binding(d->baselineProp);

    QQuickItemPrivate *targetPrivate = QQuickItemPrivate::get(d->target);
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

void QQuickAnchorChanges::copyOriginals(QDeclarativeActionEvent *other)
{
    Q_D(QQuickAnchorChanges);
    QQuickAnchorChanges *ac = static_cast<QQuickAnchorChanges*>(other);
    QQuickAnchorChangesPrivate *acp = ac->d_func();

    QQuickAnchors::Anchors combined = acp->anchorSet->d_func()->usedAnchors |
                                            acp->anchorSet->d_func()->resetAnchors;

    //probably also need to revert some things
    d->applyOrigLeft = (combined & QQuickAnchors::LeftAnchor);
    d->applyOrigRight = (combined & QQuickAnchors::RightAnchor);
    d->applyOrigHCenter = (combined & QQuickAnchors::HCenterAnchor);
    d->applyOrigTop = (combined & QQuickAnchors::TopAnchor);
    d->applyOrigBottom = (combined & QQuickAnchors::BottomAnchor);
    d->applyOrigVCenter = (combined & QQuickAnchors::VCenterAnchor);
    d->applyOrigBaseline = (combined & QQuickAnchors::BaselineAnchor);

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

void QQuickAnchorChanges::clearBindings()
{
    Q_D(QQuickAnchorChanges);
    if (!d->target)
        return;

    //### should this (saving "from" values) be moved to saveCurrentValues()?
    d->fromX = d->target->x();
    d->fromY = d->target->y();
    d->fromWidth = d->target->width();
    d->fromHeight = d->target->height();

    QQuickItemPrivate *targetPrivate = QQuickItemPrivate::get(d->target);
    //reset any anchors with corresponding reverts
    //reset any anchors that have been specified as "undefined"
    //reset any anchors that we'll be setting in the state
    QQuickAnchors::Anchors combined = d->anchorSet->d_func()->resetAnchors |
                                            d->anchorSet->d_func()->usedAnchors;
    if (d->applyOrigLeft || (combined & QQuickAnchors::LeftAnchor)) {
        targetPrivate->anchors()->resetLeft();
        QDeclarativePropertyPrivate::setBinding(d->leftProp, 0);
    }
    if (d->applyOrigRight || (combined & QQuickAnchors::RightAnchor)) {
        targetPrivate->anchors()->resetRight();
        QDeclarativePropertyPrivate::setBinding(d->rightProp, 0);
    }
    if (d->applyOrigHCenter || (combined & QQuickAnchors::HCenterAnchor)) {
        targetPrivate->anchors()->resetHorizontalCenter();
        QDeclarativePropertyPrivate::setBinding(d->hCenterProp, 0);
    }
    if (d->applyOrigTop || (combined & QQuickAnchors::TopAnchor)) {
        targetPrivate->anchors()->resetTop();
        QDeclarativePropertyPrivate::setBinding(d->topProp, 0);
    }
    if (d->applyOrigBottom || (combined & QQuickAnchors::BottomAnchor)) {
        targetPrivate->anchors()->resetBottom();
        QDeclarativePropertyPrivate::setBinding(d->bottomProp, 0);
    }
    if (d->applyOrigVCenter || (combined & QQuickAnchors::VCenterAnchor)) {
        targetPrivate->anchors()->resetVerticalCenter();
        QDeclarativePropertyPrivate::setBinding(d->vCenterProp, 0);
    }
    if (d->applyOrigBaseline || (combined & QQuickAnchors::BaselineAnchor)) {
        targetPrivate->anchors()->resetBaseline();
        QDeclarativePropertyPrivate::setBinding(d->baselineProp, 0);
    }
}

bool QQuickAnchorChanges::override(QDeclarativeActionEvent*other)
{
    if (other->typeName() != QLatin1String("AnchorChanges"))
        return false;
    if (static_cast<QDeclarativeActionEvent*>(this) == other)
        return true;
    if (static_cast<QQuickAnchorChanges*>(other)->object() == object())
        return true;
    return false;
}

void QQuickAnchorChanges::rewind()
{
    Q_D(QQuickAnchorChanges);
    if (!d->target)
        return;

    QQuickItemPrivate *targetPrivate = QQuickItemPrivate::get(d->target);

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

void QQuickAnchorChanges::saveCurrentValues()
{
    Q_D(QQuickAnchorChanges);
    if (!d->target)
        return;

    QQuickItemPrivate *targetPrivate = QQuickItemPrivate::get(d->target);
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

void QQuickAnchorChanges::saveTargetValues()
{
    Q_D(QQuickAnchorChanges);
    if (!d->target)
        return;

    d->toX = d->target->x();
    d->toY = d->target->y();
    d->toWidth = d->target->width();
    d->toHeight = d->target->height();
}

#include <moc_qquickstateoperations_p.cpp>

QT_END_NAMESPACE

