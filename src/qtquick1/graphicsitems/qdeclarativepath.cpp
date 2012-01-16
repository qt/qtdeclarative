/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
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

#include "QtQuick1/private/qdeclarativepath_p.h"
#include "QtQuick1/private/qdeclarativepath_p_p.h"

#include <QSet>
#include <QTime>

#include <private/qbezier_p.h>
#include <QtCore/qmath.h>
#include <QtCore/qnumeric.h>

QT_BEGIN_NAMESPACE



/*!
    \qmlclass PathElement QDeclarative1PathElement
    \inqmlmodule QtQuick 1
    \ingroup qml-view-elements
    \since QtQuick 1.0
    \brief PathElement is the base path type.

    This type is the base for all path types.  It cannot
    be instantiated.

    \sa Path, PathAttribute, PathPercent, PathLine, PathQuad, PathCubic
*/

/*!
    \qmlclass Path QDeclarative1Path
    \inqmlmodule QtQuick 1
    \ingroup qml-view-elements
    \since QtQuick 1.0
    \brief A Path object defines a path for use by \l PathView.

    A Path is composed of one or more path segments - PathLine, PathQuad,
    PathCubic.

    The spacing of the items along the Path can be adjusted via a
    PathPercent object.

    PathAttribute allows named attributes with values to be defined
    along the path.

    \sa PathView, PathAttribute, PathPercent, PathLine, PathQuad, PathCubic
*/
QDeclarative1Path::QDeclarative1Path(QObject *parent)
 : QObject(*(new QDeclarative1PathPrivate), parent)
{
}

QDeclarative1Path::~QDeclarative1Path()
{
}

/*!
    \qmlproperty real QtQuick1::Path::startX
    \qmlproperty real QtQuick1::Path::startY
    These properties hold the starting position of the path.
*/
qreal QDeclarative1Path::startX() const
{
    Q_D(const QDeclarative1Path);
    return d->startX;
}

void QDeclarative1Path::setStartX(qreal x)
{
    Q_D(QDeclarative1Path);
    if (qFuzzyCompare(x, d->startX))
        return;
    d->startX = x;
    emit startXChanged();
    processPath();
}

qreal QDeclarative1Path::startY() const
{
    Q_D(const QDeclarative1Path);
    return d->startY;
}

void QDeclarative1Path::setStartY(qreal y)
{
    Q_D(QDeclarative1Path);
    if (qFuzzyCompare(y, d->startY))
        return;
    d->startY = y;
    emit startYChanged();
    processPath();
}

/*!
    \qmlproperty bool QtQuick1::Path::closed
    This property holds whether the start and end of the path are identical.
*/
bool QDeclarative1Path::isClosed() const
{
    Q_D(const QDeclarative1Path);
    return d->closed;
}

/*!
    \qmlproperty list<PathElement> QtQuick1::Path::pathElements
    This property holds the objects composing the path.

    \default

    A path can contain the following path objects:
    \list
        \i \l PathLine - a straight line to a given position.
        \i \l PathQuad - a quadratic Bezier curve to a given position with a control point.
        \i \l PathCubic - a cubic Bezier curve to a given position with two control points.
        \i \l PathAttribute - an attribute at a given position in the path.
        \i \l PathPercent - a way to spread out items along various segments of the path.
    \endlist

    \snippet doc/src/snippets/qtquick1/pathview/pathattributes.qml 2
*/

QDeclarativeListProperty<QDeclarative1PathElement> QDeclarative1Path::pathElements()
{
    Q_D(QDeclarative1Path);
    return QDeclarativeListProperty<QDeclarative1PathElement>(this, d->_pathElements);
}

void QDeclarative1Path::interpolate(int idx, const QString &name, qreal value)
{
    Q_D(QDeclarative1Path);
    if (!idx)
        return;

    qreal lastValue = 0;
    qreal lastPercent = 0;
    int search = idx - 1;
    while(search >= 0) {
        const AttributePoint &point = d->_attributePoints.at(search);
        if (point.values.contains(name)) {
            lastValue = point.values.value(name);
            lastPercent = point.origpercent;
            break;
        }
        --search;
    }

    ++search;

    const AttributePoint &curPoint = d->_attributePoints.at(idx);

    for (int ii = search; ii < idx; ++ii) {
        AttributePoint &point = d->_attributePoints[ii];

        qreal val = lastValue + (value - lastValue) * (point.origpercent - lastPercent) / (curPoint.origpercent - lastPercent);
        point.values.insert(name, val);
    }
}

void QDeclarative1Path::endpoint(const QString &name)
{
    Q_D(QDeclarative1Path);
    const AttributePoint &first = d->_attributePoints.first();
    qreal val = first.values.value(name);
    for (int ii = d->_attributePoints.count() - 1; ii >= 0; ii--) {
        const AttributePoint &point = d->_attributePoints.at(ii);
        if (point.values.contains(name)) {
            for (int jj = ii + 1; jj < d->_attributePoints.count(); ++jj) {
                AttributePoint &setPoint = d->_attributePoints[jj];
                setPoint.values.insert(name, val);
            }
            return;
        }
    }
}

void QDeclarative1Path::processPath()
{
    Q_D(QDeclarative1Path);

    if (!d->componentComplete)
        return;

    d->_pointCache.clear();
    d->_attributePoints.clear();
    d->_path = QPainterPath();

    AttributePoint first;
    for (int ii = 0; ii < d->_attributes.count(); ++ii)
        first.values[d->_attributes.at(ii)] = 0;
    d->_attributePoints << first;

    d->_path.moveTo(d->startX, d->startY);

    QDeclarative1Curve *lastCurve = 0;
    foreach (QDeclarative1PathElement *pathElement, d->_pathElements) {
        if (QDeclarative1Curve *curve = qobject_cast<QDeclarative1Curve *>(pathElement)) {
            curve->addToPath(d->_path);
            AttributePoint p;
            p.origpercent = d->_path.length();
            d->_attributePoints << p;
            lastCurve = curve;
        } else if (QDeclarative1PathAttribute *attribute = qobject_cast<QDeclarative1PathAttribute *>(pathElement)) {
            AttributePoint &point = d->_attributePoints.last();
            point.values[attribute->name()] = attribute->value();
            interpolate(d->_attributePoints.count() - 1, attribute->name(), attribute->value());
        } else if (QDeclarative1PathPercent *percent = qobject_cast<QDeclarative1PathPercent *>(pathElement)) {
            AttributePoint &point = d->_attributePoints.last();
            point.values[QLatin1String("_qfx_percent")] = percent->value();
            interpolate(d->_attributePoints.count() - 1, QLatin1String("_qfx_percent"), percent->value());
        }
    }

    // Fixup end points
    const AttributePoint &last = d->_attributePoints.last();
    for (int ii = 0; ii < d->_attributes.count(); ++ii) {
        if (!last.values.contains(d->_attributes.at(ii)))
            endpoint(d->_attributes.at(ii));
    }

    // Adjust percent
    qreal length = d->_path.length();
    qreal prevpercent = 0;
    qreal prevorigpercent = 0;
    for (int ii = 0; ii < d->_attributePoints.count(); ++ii) {
        const AttributePoint &point = d->_attributePoints.at(ii);
        if (point.values.contains(QLatin1String("_qfx_percent"))) { //special string for QDeclarative1PathPercent
            if ( ii > 0) {
                qreal scale = (d->_attributePoints[ii].origpercent/length - prevorigpercent) /
                            (point.values.value(QLatin1String("_qfx_percent"))-prevpercent);
                d->_attributePoints[ii].scale = scale;
            }
            d->_attributePoints[ii].origpercent /= length;
            d->_attributePoints[ii].percent = point.values.value(QLatin1String("_qfx_percent"));
            prevorigpercent = d->_attributePoints[ii].origpercent;
            prevpercent = d->_attributePoints[ii].percent;
        } else {
            d->_attributePoints[ii].origpercent /= length;
            d->_attributePoints[ii].percent = d->_attributePoints[ii].origpercent;
        }
    }

    d->closed = lastCurve && d->startX == lastCurve->x() && d->startY == lastCurve->y();

    emit changed();
}

void QDeclarative1Path::classBegin()
{
    Q_D(QDeclarative1Path);
    d->componentComplete = false;
}

void QDeclarative1Path::componentComplete()
{
    Q_D(QDeclarative1Path);
    QSet<QString> attrs;
    d->componentComplete = true;

    // First gather up all the attributes
    foreach (QDeclarative1PathElement *pathElement, d->_pathElements) {
        if (QDeclarative1PathAttribute *attribute =
            qobject_cast<QDeclarative1PathAttribute *>(pathElement))
            attrs.insert(attribute->name());
    }
    d->_attributes = attrs.toList();

    processPath();

    foreach (QDeclarative1PathElement *pathElement, d->_pathElements)
        connect(pathElement, SIGNAL(changed()), this, SLOT(processPath()));
}

QPainterPath QDeclarative1Path::path() const
{
    Q_D(const QDeclarative1Path);
    return d->_path;
}

QStringList QDeclarative1Path::attributes() const
{
    Q_D(const QDeclarative1Path);
    if (!d->componentComplete) {
        QSet<QString> attrs;

        // First gather up all the attributes
        foreach (QDeclarative1PathElement *pathElement, d->_pathElements) {
            if (QDeclarative1PathAttribute *attribute =
                qobject_cast<QDeclarative1PathAttribute *>(pathElement))
                attrs.insert(attribute->name());
        }
        return attrs.toList();
    }
    return d->_attributes;
}

static inline QBezier nextBezier(const QPainterPath &path, int *from, qreal *bezLength)
{
    const int lastElement = path.elementCount() - 1;
    for (int i=*from; i <= lastElement; ++i) {
        const QPainterPath::Element &e = path.elementAt(i);

        switch (e.type) {
        case QPainterPath::MoveToElement:
            break;
        case QPainterPath::LineToElement:
        {
            QLineF line(path.elementAt(i-1), e);
            *bezLength = line.length();
            QPointF a = path.elementAt(i-1);
            QPointF delta = e - a;
            *from = i+1;
            return QBezier::fromPoints(a, a + delta / 3, a + 2 * delta / 3, e);
        }
        case QPainterPath::CurveToElement:
        {
            QBezier b = QBezier::fromPoints(path.elementAt(i-1),
                                            e,
                                            path.elementAt(i+1),
                                            path.elementAt(i+2));
            *bezLength = b.length();
            *from = i+3;
            return b;
        }
        default:
            break;
        }
    }
    *from = lastElement;
    *bezLength = 0;
    return QBezier();
}

void QDeclarative1Path::createPointCache() const
{
    Q_D(const QDeclarative1Path);
    qreal pathLength = d->_path.length();
    if (pathLength <= 0 || qIsNaN(pathLength))
        return;
    // more points means less jitter between items as they move along the
    // path, but takes longer to generate
    const int points = qCeil(pathLength*5);
    const int lastElement = d->_path.elementCount() - 1;
    d->_pointCache.resize(points+1);

    int currElement = 0;
    qreal bezLength = 0;
    QBezier currBez = nextBezier(d->_path, &currElement, &bezLength);
    qreal currLength = bezLength;
    qreal epc = currLength / pathLength;

    for (int i = 0; i < d->_pointCache.size(); i++) {
        //find which set we are in
        qreal prevPercent = 0;
        qreal prevOrigPercent = 0;
        for (int ii = 0; ii < d->_attributePoints.count(); ++ii) {
            qreal percent = qreal(i)/points;
            const AttributePoint &point = d->_attributePoints.at(ii);
            if (percent < point.percent || ii == d->_attributePoints.count() - 1) { //### || is special case for very last item
                qreal elementPercent = (percent - prevPercent);

                qreal spc = prevOrigPercent + elementPercent * point.scale;

                while (spc > epc) {
                    if (currElement > lastElement)
                        break;
                    currBez = nextBezier(d->_path, &currElement, &bezLength);
                    if (bezLength == 0.0) {
                        currLength = pathLength;
                        epc = 1.0;
                        break;
                    }
                    currLength += bezLength;
                    epc = currLength / pathLength;
                }
                qreal realT = (pathLength * spc - (currLength - bezLength)) / bezLength;
                d->_pointCache[i] = currBez.pointAt(qBound(qreal(0), realT, qreal(1)));
                break;
            }
            prevOrigPercent = point.origpercent;
            prevPercent = point.percent;
        }
    }
}

QPointF QDeclarative1Path::pointAt(qreal p) const
{
    Q_D(const QDeclarative1Path);
    if (d->_pointCache.isEmpty()) {
        createPointCache();
        if (d->_pointCache.isEmpty())
            return QPointF();
    }
    int idx = qRound(p*d->_pointCache.size());
    if (idx >= d->_pointCache.size())
        idx = d->_pointCache.size() - 1;
    else if (idx < 0)
        idx = 0;
    return d->_pointCache.at(idx);
}

qreal QDeclarative1Path::attributeAt(const QString &name, qreal percent) const
{
    Q_D(const QDeclarative1Path);
    if (percent < 0 || percent > 1)
        return 0;

    for (int ii = 0; ii < d->_attributePoints.count(); ++ii) {
        const AttributePoint &point = d->_attributePoints.at(ii);

        if (point.percent == percent) {
            return point.values.value(name);
        } else if (point.percent > percent) {
            qreal lastValue =
                ii?(d->_attributePoints.at(ii - 1).values.value(name)):0;
            qreal lastPercent =
                ii?(d->_attributePoints.at(ii - 1).percent):0;
            qreal curValue = point.values.value(name);
            qreal curPercent = point.percent;

            return lastValue + (curValue - lastValue) * (percent - lastPercent) / (curPercent - lastPercent);
        }
    }

    return 0;
}

/****************************************************************************/

qreal QDeclarative1Curve::x() const
{
    return _x;
}

void QDeclarative1Curve::setX(qreal x)
{
    if (_x != x) {
        _x = x;
        emit xChanged();
        emit changed();
    }
}

qreal QDeclarative1Curve::y() const
{
    return _y;
}

void QDeclarative1Curve::setY(qreal y)
{
    if (_y != y) {
        _y = y;
        emit yChanged();
        emit changed();
    }
}

/****************************************************************************/

/*!
    \qmlclass PathAttribute QDeclarative1PathAttribute
    \inqmlmodule QtQuick 1
    \ingroup qml-view-elements
    \since QtQuick 1.0
    \brief The PathAttribute allows setting an attribute at a given position in a Path.

    The PathAttribute object allows attributes consisting of a name and
    a value to be specified for various points along a path.  The
    attributes are exposed to the delegate as
    \l{qdeclarativeintroduction.html#attached-properties} {Attached Properties}.
    The value of an attribute at any particular point along the path is interpolated
    from the PathAttributes bounding that point.

    The example below shows a path with the items scaled to 30% with
    opacity 50% at the top of the path and scaled 100% with opacity
    100% at the bottom.  Note the use of the PathView.iconScale and
    PathView.iconOpacity attached properties to set the scale and opacity
    of the delegate.

    \table
    \row
    \o \image declarative-pathattribute.png
    \o
    \snippet doc/src/snippets/qtquick1/pathview/pathattributes.qml 0
    (see the PathView documentation for the specification of ContactModel.qml
     used for ContactModel above.)
    \endtable


    \sa Path
*/

/*!
    \qmlproperty string QtQuick1::PathAttribute::name
    This property holds the name of the attribute to change.

    This attribute will be available to the delegate as PathView.<name>

    Note that using an existing Item property name such as "opacity" as an
    attribute is allowed.  This is because path attributes add a new
    \l{qdeclarativeintroduction.html#attached-properties} {Attached Property}
    which in no way clashes with existing properties.
*/

/*!
    the name of the attribute to change.
*/

QString QDeclarative1PathAttribute::name() const
{
    return _name;
}

void QDeclarative1PathAttribute::setName(const QString &name)
{
    if (_name == name)
        return;
     _name = name;
    emit nameChanged();
}

/*!
   \qmlproperty real QtQuick1::PathAttribute::value
   This property holds the value for the attribute.

   The value specified can be used to influence the visual appearance
   of an item along the path. For example, the following Path specifies
   an attribute named \e itemRotation, which has the value \e 0 at the
   beginning of the path, and the value 90 at the end of the path.

   \qml
   Path {
       startX: 0
       startY: 0
       PathAttribute { name: "itemRotation"; value: 0 }
       PathLine { x: 100; y: 100 }
       PathAttribute { name: "itemRotation"; value: 90 }
   }
   \endqml

   In our delegate, we can then bind the \e rotation property to the
   \l{qdeclarativeintroduction.html#attached-properties} {Attached Property}
   \e PathView.itemRotation created for this attribute.

   \qml
   Rectangle {
       width: 10; height: 10
       rotation: PathView.itemRotation
   }
   \endqml

   As each item is positioned along the path, it will be rotated accordingly:
   an item at the beginning of the path with be not be rotated, an item at
   the end of the path will be rotated 90 degrees, and an item mid-way along
   the path will be rotated 45 degrees.
*/

/*!
    the new value of the attribute.
*/
qreal QDeclarative1PathAttribute::value() const
{
    return _value;
}

void QDeclarative1PathAttribute::setValue(qreal value)
{
    if (_value != value) {
        _value = value;
        emit valueChanged();
        emit changed();
    }
}

/****************************************************************************/

/*!
    \qmlclass PathLine QDeclarative1PathLine
    \inqmlmodule QtQuick 1
    \ingroup qml-view-elements
    \since QtQuick 1.0
    \brief The PathLine defines a straight line.

    The example below creates a path consisting of a straight line from
    0,100 to 200,100:

    \qml
    Path {
        startX: 0; startY: 100
        PathLine { x: 200; y: 100 }
    }
    \endqml

    \sa Path, PathQuad, PathCubic
*/

/*!
    \qmlproperty real QtQuick1::PathLine::x
    \qmlproperty real QtQuick1::PathLine::y

    Defines the end point of the line.
*/

void QDeclarative1PathLine::addToPath(QPainterPath &path)
{
    path.lineTo(x(), y());
}

/****************************************************************************/

/*!
    \qmlclass PathQuad QDeclarative1PathQuad
    \inqmlmodule QtQuick 1
    \ingroup qml-view-elements
    \since QtQuick 1.0
    \brief The PathQuad defines a quadratic Bezier curve with a control point.

    The following QML produces the path shown below:
    \table
    \row
    \o \image declarative-pathquad.png
    \o
    \qml
    Path {
        startX: 0; startY: 0
        PathQuad { x: 200; y: 0; controlX: 100; controlY: 150 }
    }
    \endqml
    \endtable

    \sa Path, PathCubic, PathLine
*/

/*!
    \qmlproperty real QtQuick1::PathQuad::x
    \qmlproperty real QtQuick1::PathQuad::y

    Defines the end point of the curve.
*/

/*!
   \qmlproperty real QtQuick1::PathQuad::controlX
   \qmlproperty real QtQuick1::PathQuad::controlY

   Defines the position of the control point.
*/

/*!
    the x position of the control point.
*/
qreal QDeclarative1PathQuad::controlX() const
{
    return _controlX;
}

void QDeclarative1PathQuad::setControlX(qreal x)
{
    if (_controlX != x) {
        _controlX = x;
        emit controlXChanged();
        emit changed();
    }
}


/*!
    the y position of the control point.
*/
qreal QDeclarative1PathQuad::controlY() const
{
    return _controlY;
}

void QDeclarative1PathQuad::setControlY(qreal y)
{
    if (_controlY != y) {
        _controlY = y;
        emit controlYChanged();
        emit changed();
    }
}

void QDeclarative1PathQuad::addToPath(QPainterPath &path)
{
    path.quadTo(controlX(), controlY(), x(), y());
}

/****************************************************************************/

/*!
   \qmlclass PathCubic QDeclarative1PathCubic
    \inqmlmodule QtQuick 1
    \ingroup qml-view-elements
    \since QtQuick 1.0
   \brief The PathCubic defines a cubic Bezier curve with two control points.

    The following QML produces the path shown below:
    \table
    \row
    \o \image declarative-pathcubic.png
    \o
    \qml
    Path {
        startX: 20; startY: 0
        PathCubic {
            x: 180; y: 0
            control1X: -10; control1Y: 90
            control2X: 210; control2Y: 90
        }
    }
    \endqml
    \endtable

    \sa Path, PathQuad, PathLine
*/

/*!
    \qmlproperty real QtQuick1::PathCubic::x
    \qmlproperty real QtQuick1::PathCubic::y

    Defines the end point of the curve.
*/

/*!
   \qmlproperty real QtQuick1::PathCubic::control1X
   \qmlproperty real QtQuick1::PathCubic::control1Y

    Defines the position of the first control point.
*/
qreal QDeclarative1PathCubic::control1X() const
{
    return _control1X;
}

void QDeclarative1PathCubic::setControl1X(qreal x)
{
    if (_control1X != x) {
        _control1X = x;
        emit control1XChanged();
        emit changed();
    }
}

qreal QDeclarative1PathCubic::control1Y() const
{
    return _control1Y;
}

void QDeclarative1PathCubic::setControl1Y(qreal y)
{
    if (_control1Y != y) {
        _control1Y = y;
        emit control1YChanged();
        emit changed();
    }
}

/*!
   \qmlproperty real QtQuick1::PathCubic::control2X
   \qmlproperty real QtQuick1::PathCubic::control2Y

    Defines the position of the second control point.
*/
qreal QDeclarative1PathCubic::control2X() const
{
    return _control2X;
}

void QDeclarative1PathCubic::setControl2X(qreal x)
{
    if (_control2X != x) {
        _control2X = x;
        emit control2XChanged();
        emit changed();
    }
}

qreal QDeclarative1PathCubic::control2Y() const
{
    return _control2Y;
}

void QDeclarative1PathCubic::setControl2Y(qreal y)
{
    if (_control2Y != y) {
        _control2Y = y;
        emit control2YChanged();
        emit changed();
    }
}

void QDeclarative1PathCubic::addToPath(QPainterPath &path)
{
    path.cubicTo(control1X(), control1Y(), control2X(), control2Y(), x(), y());
}

/****************************************************************************/

/*!
    \qmlclass PathPercent QDeclarative1PathPercent
    \inqmlmodule QtQuick 1
    \ingroup qml-view-elements
    \since QtQuick 1.0
    \brief The PathPercent manipulates the way a path is interpreted.

    PathPercent allows you to manipulate the spacing between items on a
    PathView's path. You can use it to bunch together items on part of
    the path, and spread them out on other parts of the path.

    The examples below show the normal distrubution of items along a path
    compared to a distribution which places 50% of the items along the
    PathLine section of the path.
    \table
    \row
    \o \image declarative-nopercent.png
    \o
    \qml
    PathView {
        // ...
        Path {
            startX: 20; startY: 0
            PathQuad { x: 50; y: 80; controlX: 0; controlY: 80 }
            PathLine { x: 150; y: 80 }
            PathQuad { x: 180; y: 0; controlX: 200; controlY: 80 }
        }
    }
    \endqml
    \row
    \o \image declarative-percent.png
    \o
    \qml
    PathView {
        // ...
        Path {
            startX: 20; startY: 0
            PathQuad { x: 50; y: 80; controlX: 0; controlY: 80 }
            PathPercent { value: 0.25 }
            PathLine { x: 150; y: 80 }
            PathPercent { value: 0.75 }
            PathQuad { x: 180; y: 0; controlX: 200; controlY: 80 }
            PathPercent { value: 1 }
        }
    }
    \endqml
    \endtable

    \sa Path
*/

/*!
    \qmlproperty real QtQuick1::PathPercent::value
    The proporation of items that should be laid out up to this point.

    This value should always be higher than the last value specified
    by a PathPercent at a previous position in the Path.

    In the following example we have a Path made up of three PathLines.
    Normally, the items of the PathView would be laid out equally along
    this path, with an equal number of items per line segment. PathPercent
    allows us to specify that the first and third lines should each hold
    10% of the laid out items, while the second line should hold the remaining
    80%.

    \qml
    PathView {
        // ...
        Path {
            startX: 0; startY: 0
            PathLine { x:100; y: 0; }
            PathPercent { value: 0.1 }
            PathLine { x: 100; y: 100 }
            PathPercent { value: 0.9 }
            PathLine { x: 100; y: 0 }
            PathPercent { value: 1 }
        }
    }
    \endqml
*/

qreal QDeclarative1PathPercent::value() const
{
    return _value;
}

void QDeclarative1PathPercent::setValue(qreal value)
{
    if (_value != value) {
        _value = value;
        emit valueChanged();
        emit changed();
    }
}


QT_END_NAMESPACE
