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

#include "qdeclarativepath_p.h"
#include "qdeclarativepath_p_p.h"
#include "qdeclarativesvgparser_p.h"

#include <QSet>
#include <QTime>

#include <private/qbezier_p.h>
#include <QtCore/qmath.h>
#include <QtCore/qnumeric.h>

QT_BEGIN_NAMESPACE

/*!
    \qmlclass PathElement QDeclarativePathElement
    \inqmlmodule QtQuick 2
    \ingroup qml-view-elements
    \brief PathElement is the base path type.

    This type is the base for all path types.  It cannot
    be instantiated.

    \sa Path, PathAttribute, PathPercent, PathLine, PathQuad, PathCubic
*/

/*!
    \qmlclass Path QDeclarativePath
    \inqmlmodule QtQuick 2
    \ingroup qml-view-elements
    \brief A Path object defines a path for use by \l PathView.

    A Path is composed of one or more path segments - PathLine, PathQuad,
    PathCubic.

    The spacing of the items along the Path can be adjusted via a
    PathPercent object.

    PathAttribute allows named attributes with values to be defined
    along the path.

    \sa PathView, PathAttribute, PathPercent, PathLine, PathQuad, PathCubic
*/
QDeclarativePath::QDeclarativePath(QObject *parent)
 : QObject(*(new QDeclarativePathPrivate), parent)
{
}

QDeclarativePath::~QDeclarativePath()
{
}

/*!
    \qmlproperty real QtQuick2::Path::startX
    \qmlproperty real QtQuick2::Path::startY
    These properties hold the starting position of the path.
*/
qreal QDeclarativePath::startX() const
{
    Q_D(const QDeclarativePath);
    return d->startX.isNull ? 0 : d->startX.value;
}

void QDeclarativePath::setStartX(qreal x)
{
    Q_D(QDeclarativePath);
    if (d->startX.isValid() && qFuzzyCompare(x, d->startX))
        return;
    d->startX = x;
    emit startXChanged();
    processPath();
}

bool QDeclarativePath::hasStartX() const
{
    Q_D(const QDeclarativePath);
    return d->startX.isValid();
}

qreal QDeclarativePath::startY() const
{
    Q_D(const QDeclarativePath);
    return d->startY.isNull ? 0 : d->startY.value;
}

void QDeclarativePath::setStartY(qreal y)
{
    Q_D(QDeclarativePath);
    if (d->startY.isValid() && qFuzzyCompare(y, d->startY))
        return;
    d->startY = y;
    emit startYChanged();
    processPath();
}

bool QDeclarativePath::hasStartY() const
{
    Q_D(const QDeclarativePath);
    return d->startY.isValid();
}

/*!
    \qmlproperty bool QtQuick2::Path::closed
    This property holds whether the start and end of the path are identical.
*/
bool QDeclarativePath::isClosed() const
{
    Q_D(const QDeclarativePath);
    return d->closed;
}

bool QDeclarativePath::hasEnd() const
{
    Q_D(const QDeclarativePath);
    for (int i = d->_pathElements.count() - 1; i > -1; --i) {
        if (QDeclarativeCurve *curve = qobject_cast<QDeclarativeCurve *>(d->_pathElements.at(i))) {
            if ((!curve->hasX() && !curve->hasRelativeX()) || (!curve->hasY() && !curve->hasRelativeY()))
                return false;
            else
                return true;
        }
    }
    return hasStartX() && hasStartY();
}

/*!
    \qmlproperty list<PathElement> QtQuick2::Path::pathElements
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

    \snippet doc/src/snippets/declarative/pathview/pathattributes.qml 2
*/

QDeclarativeListProperty<QDeclarativePathElement> QDeclarativePath::pathElements()
{
    Q_D(QDeclarativePath);
    return QDeclarativeListProperty<QDeclarativePathElement>(this, d->_pathElements);
}

void QDeclarativePath::interpolate(int idx, const QString &name, qreal value)
{
    Q_D(QDeclarativePath);
    interpolate(d->_attributePoints, idx, name, value);
}

void QDeclarativePath::interpolate(QList<AttributePoint> &attributePoints, int idx, const QString &name, qreal value)
{
    if (!idx)
        return;

    qreal lastValue = 0;
    qreal lastPercent = 0;
    int search = idx - 1;
    while(search >= 0) {
        const AttributePoint &point = attributePoints.at(search);
        if (point.values.contains(name)) {
            lastValue = point.values.value(name);
            lastPercent = point.origpercent;
            break;
        }
        --search;
    }

    ++search;

    const AttributePoint &curPoint = attributePoints.at(idx);

    for (int ii = search; ii < idx; ++ii) {
        AttributePoint &point = attributePoints[ii];

        qreal val = lastValue + (value - lastValue) * (point.origpercent - lastPercent) / (curPoint.origpercent - lastPercent);
        point.values.insert(name, val);
    }
}

void QDeclarativePath::endpoint(const QString &name)
{
    Q_D(QDeclarativePath);
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

void QDeclarativePath::endpoint(QList<AttributePoint> &attributePoints, const QString &name)
{
    const AttributePoint &first = attributePoints.first();
    qreal val = first.values.value(name);
    for (int ii = attributePoints.count() - 1; ii >= 0; ii--) {
        const AttributePoint &point = attributePoints.at(ii);
        if (point.values.contains(name)) {
            for (int jj = ii + 1; jj < attributePoints.count(); ++jj) {
                AttributePoint &setPoint = attributePoints[jj];
                setPoint.values.insert(name, val);
            }
            return;
        }
    }
}

static QString percentString(QStringLiteral("_qfx_percent"));

void QDeclarativePath::processPath()
{
    Q_D(QDeclarativePath);

    if (!d->componentComplete)
        return;

    d->_pointCache.clear();
    d->prevBez.isValid = false;

    d->_path = createPath(QPointF(), QPointF(), d->_attributes, d->pathLength, d->_attributePoints, &d->closed);

    emit changed();
}

QPainterPath QDeclarativePath::createPath(const QPointF &startPoint, const QPointF &endPoint, const QStringList &attributes, qreal &pathLength, QList<AttributePoint> &attributePoints, bool *closed)
{
    Q_D(QDeclarativePath);

    pathLength = 0;
    attributePoints.clear();

    if (!d->componentComplete)
        return QPainterPath();

    QPainterPath path;

    AttributePoint first;
    for (int ii = 0; ii < attributes.count(); ++ii)
        first.values[attributes.at(ii)] = 0;
    attributePoints << first;

    qreal startX = d->startX.isValid() ? d->startX.value : startPoint.x();
    qreal startY = d->startY.isValid() ? d->startY.value : startPoint.y();
    path.moveTo(startX, startY);

    bool usesPercent = false;
    int index = 0;
    foreach (QDeclarativePathElement *pathElement, d->_pathElements) {
        if (QDeclarativeCurve *curve = qobject_cast<QDeclarativeCurve *>(pathElement)) {
            QDeclarativePathData data;
            data.index = index;
            data.endPoint = endPoint;
            data.curves = d->_pathCurves;
            curve->addToPath(path, data);
            AttributePoint p;
            p.origpercent = path.length();
            attributePoints << p;
            ++index;
        } else if (QDeclarativePathAttribute *attribute = qobject_cast<QDeclarativePathAttribute *>(pathElement)) {
            AttributePoint &point = attributePoints.last();
            point.values[attribute->name()] = attribute->value();
            interpolate(attributePoints, attributePoints.count() - 1, attribute->name(), attribute->value());
        } else if (QDeclarativePathPercent *percent = qobject_cast<QDeclarativePathPercent *>(pathElement)) {
            AttributePoint &point = attributePoints.last();
            point.values[percentString] = percent->value();
            interpolate(attributePoints, attributePoints.count() - 1, percentString, percent->value());
            usesPercent = true;
        }
    }

    // Fixup end points
    const AttributePoint &last = attributePoints.last();
    for (int ii = 0; ii < attributes.count(); ++ii) {
        if (!last.values.contains(attributes.at(ii)))
            endpoint(attributePoints, attributes.at(ii));
    }
    if (usesPercent && !last.values.contains(percentString)) {
        d->_attributePoints.last().values[percentString] = 1;
        interpolate(d->_attributePoints.count() - 1, percentString, 1);
    }


    // Adjust percent
    qreal length = path.length();
    qreal prevpercent = 0;
    qreal prevorigpercent = 0;
    for (int ii = 0; ii < attributePoints.count(); ++ii) {
        const AttributePoint &point = attributePoints.at(ii);
        if (point.values.contains(percentString)) { //special string for QDeclarativePathPercent
            if ( ii > 0) {
                qreal scale = (attributePoints[ii].origpercent/length - prevorigpercent) /
                            (point.values.value(percentString)-prevpercent);
                attributePoints[ii].scale = scale;
            }
            attributePoints[ii].origpercent /= length;
            attributePoints[ii].percent = point.values.value(percentString);
            prevorigpercent = attributePoints[ii].origpercent;
            prevpercent = attributePoints[ii].percent;
        } else {
            attributePoints[ii].origpercent /= length;
            attributePoints[ii].percent = attributePoints[ii].origpercent;
        }
    }

    if (closed) {
        QPointF end = path.currentPosition();
        *closed = length > 0 && startX == end.x() && startY == end.y();
    }
    pathLength = length;

    return path;
}

void QDeclarativePath::classBegin()
{
    Q_D(QDeclarativePath);
    d->componentComplete = false;
}

void QDeclarativePath::componentComplete()
{
    Q_D(QDeclarativePath);
    QSet<QString> attrs;
    d->componentComplete = true;

    // First gather up all the attributes
    foreach (QDeclarativePathElement *pathElement, d->_pathElements) {
        if (QDeclarativeCurve *curve =
            qobject_cast<QDeclarativeCurve *>(pathElement))
            d->_pathCurves.append(curve);
        else if (QDeclarativePathAttribute *attribute =
                 qobject_cast<QDeclarativePathAttribute *>(pathElement))
            attrs.insert(attribute->name());
    }
    d->_attributes = attrs.toList();

    processPath();

    foreach (QDeclarativePathElement *pathElement, d->_pathElements)
        connect(pathElement, SIGNAL(changed()), this, SLOT(processPath()));
}

QPainterPath QDeclarativePath::path() const
{
    Q_D(const QDeclarativePath);
    return d->_path;
}

QStringList QDeclarativePath::attributes() const
{
    Q_D(const QDeclarativePath);
    if (!d->componentComplete) {
        QSet<QString> attrs;

        // First gather up all the attributes
        foreach (QDeclarativePathElement *pathElement, d->_pathElements) {
            if (QDeclarativePathAttribute *attribute =
                qobject_cast<QDeclarativePathAttribute *>(pathElement))
                attrs.insert(attribute->name());
        }
        return attrs.toList();
    }
    return d->_attributes;
}

static inline QBezier nextBezier(const QPainterPath &path, int *current, qreal *bezLength, bool reverse = false)
{
    const int lastElement = reverse ? 0 : path.elementCount() - 1;
    const int start = reverse ? *current - 1 : *current + 1;
    for (int i=start; reverse ? i >= lastElement : i <= lastElement; reverse ? --i : ++i) {
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
            *current = i;
            return QBezier::fromPoints(a, a + delta / 3, a + 2 * delta / 3, e);
        }
        case QPainterPath::CurveToElement:
        {
            QBezier b = QBezier::fromPoints(path.elementAt(i-1),
                                            e,
                                            path.elementAt(i+1),
                                            path.elementAt(i+2));
            *bezLength = b.length();
            *current = i;
            return b;
        }
        default:
            break;
        }
    }
    *current = lastElement;
    *bezLength = 0;
    return QBezier();
}

//derivative of the equation
static inline qreal slopeAt(qreal t, qreal a, qreal b, qreal c, qreal d)
{
    return 3*t*t*(d - 3*c + 3*b - a) + 6*t*(c - 2*b + a) + 3*(b - a);
}

void QDeclarativePath::createPointCache() const
{
    Q_D(const QDeclarativePath);
    qreal pathLength = d->pathLength;
    if (pathLength <= 0 || qIsNaN(pathLength))
        return;
    // more points means less jitter between items as they move along the
    // path, but takes longer to generate
    const int points = qCeil(pathLength*5);
    const int lastElement = d->_path.elementCount() - 1;
    d->_pointCache.resize(points+1);

    int currElement = -1;
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

QPointF QDeclarativePath::sequentialPointAt(qreal p, qreal *angle) const
{
    Q_D(const QDeclarativePath);
    return sequentialPointAt(d->_path, d->pathLength, d->_attributePoints, d->prevBez, p, angle);
}

QPointF QDeclarativePath::sequentialPointAt(const QPainterPath &path, const qreal &pathLength, const QList<AttributePoint> &attributePoints, QDeclarativeCachedBezier &prevBez, qreal p, qreal *angle)
{
    if (!prevBez.isValid)
        return p > .5 ? backwardsPointAt(path, pathLength, attributePoints, prevBez, p, angle) :
                        forwardsPointAt(path, pathLength, attributePoints, prevBez, p, angle);

    return p < prevBez.p ? backwardsPointAt(path, pathLength, attributePoints, prevBez, p, angle) :
                           forwardsPointAt(path, pathLength, attributePoints, prevBez, p, angle);
}

QPointF QDeclarativePath::forwardsPointAt(const QPainterPath &path, const qreal &pathLength, const QList<AttributePoint> &attributePoints, QDeclarativeCachedBezier &prevBez, qreal p, qreal *angle)
{
    if (pathLength <= 0 || qIsNaN(pathLength))
        return path.pointAtPercent(0);  //expensive?

    const int lastElement = path.elementCount() - 1;
    bool haveCachedBez = prevBez.isValid;
    int currElement = haveCachedBez ? prevBez.element : -1;
    qreal bezLength = haveCachedBez ? prevBez.bezLength : 0;
    QBezier currBez = haveCachedBez ? prevBez.bezier : nextBezier(path, &currElement, &bezLength);
    qreal currLength = haveCachedBez ? prevBez.currLength : bezLength;
    qreal epc = currLength / pathLength;

    //find which set we are in
    qreal prevPercent = 0;
    qreal prevOrigPercent = 0;
    for (int ii = 0; ii < attributePoints.count(); ++ii) {
        qreal percent = p;
        const AttributePoint &point = attributePoints.at(ii);
        if (percent < point.percent || ii == attributePoints.count() - 1) {
            qreal elementPercent = (percent - prevPercent);

            qreal spc = prevOrigPercent + elementPercent * point.scale;

            while (spc > epc) {
                Q_ASSERT(!(currElement > lastElement));
                currBez = nextBezier(path, &currElement, &bezLength);
                currLength += bezLength;
                epc = currLength / pathLength;
            }
            prevBez.element = currElement;
            prevBez.bezLength = bezLength;
            prevBez.currLength = currLength;
            prevBez.bezier = currBez;
            prevBez.p = p;
            prevBez.isValid = true;

            qreal realT = (pathLength * spc - (currLength - bezLength)) / bezLength;

            if (angle) {
                qreal m1 = slopeAt(realT, currBez.x1, currBez.x2, currBez.x3, currBez.x4);
                qreal m2 = slopeAt(realT, currBez.y1, currBez.y2, currBez.y3, currBez.y4);
                *angle = QLineF(0, 0, m1, m2).angle();
            }

            return currBez.pointAt(qBound(qreal(0), realT, qreal(1)));
        }
        prevOrigPercent = point.origpercent;
        prevPercent = point.percent;
    }

    return QPointF(0,0);
}

//ideally this should be merged with forwardsPointAt
QPointF QDeclarativePath::backwardsPointAt(const QPainterPath &path, const qreal &pathLength, const QList<AttributePoint> &attributePoints, QDeclarativeCachedBezier &prevBez, qreal p, qreal *angle)
{
    if (pathLength <= 0 || qIsNaN(pathLength))
        return path.pointAtPercent(0);

    const int firstElement = 0;
    bool haveCachedBez = prevBez.isValid;
    int currElement = haveCachedBez ? prevBez.element : path.elementCount();
    qreal bezLength = haveCachedBez ? prevBez.bezLength : 0;
    QBezier currBez = haveCachedBez ? prevBez.bezier : nextBezier(path, &currElement, &bezLength, true /*reverse*/);
    qreal currLength = haveCachedBez ? prevBez.currLength : pathLength;
    qreal prevLength = currLength - bezLength;
    qreal epc = prevLength / pathLength;

    for (int ii = attributePoints.count() - 1; ii > 0; --ii) {
        qreal percent = p;
        const AttributePoint &point = attributePoints.at(ii);
        const AttributePoint &prevPoint = attributePoints.at(ii-1);
        if (percent > prevPoint.percent || ii == 1) {
            qreal elementPercent = (percent - prevPoint.percent);

            qreal spc = prevPoint.origpercent + elementPercent * point.scale;

            while (spc < epc) {
                Q_ASSERT(!(currElement < firstElement));
                currBez = nextBezier(path, &currElement, &bezLength, true /*reverse*/);
                currLength = prevLength;
                prevLength = currLength - bezLength;
                epc = prevLength / pathLength;
            }
            prevBez.element = currElement;
            prevBez.bezLength = bezLength;
            prevBez.currLength = currLength;
            prevBez.bezier = currBez;
            prevBez.p = p;
            prevBez.isValid = true;

            qreal realT = (pathLength * spc - (currLength - bezLength)) / bezLength;

            if (angle) {
                qreal m1 = slopeAt(realT, currBez.x1, currBez.x2, currBez.x3, currBez.x4);
                qreal m2 = slopeAt(realT, currBez.y1, currBez.y2, currBez.y3, currBez.y4);
                *angle = QLineF(0, 0, m1, m2).angle();
            }

            return currBez.pointAt(qBound(qreal(0), realT, qreal(1)));
        }
    }

    return QPointF(0,0);
}

QPointF QDeclarativePath::pointAt(qreal p) const
{
    Q_D(const QDeclarativePath);
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

qreal QDeclarativePath::attributeAt(const QString &name, qreal percent) const
{
    Q_D(const QDeclarativePath);
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

qreal QDeclarativeCurve::x() const
{
    return _x.isNull ? 0 : _x.value;
}

void QDeclarativeCurve::setX(qreal x)
{
    if (_x.isNull || _x != x) {
        _x = x;
        emit xChanged();
        emit changed();
    }
}

bool QDeclarativeCurve::hasX()
{
    return _x.isValid();
}

qreal QDeclarativeCurve::y() const
{
    return _y.isNull ? 0 : _y.value;
}

void QDeclarativeCurve::setY(qreal y)
{
    if (_y.isNull || _y != y) {
        _y = y;
        emit yChanged();
        emit changed();
    }
}

bool QDeclarativeCurve::hasY()
{
    return _y.isValid();
}

qreal QDeclarativeCurve::relativeX() const
{
    return _relativeX;
}

void QDeclarativeCurve::setRelativeX(qreal x)
{
    if (_relativeX.isNull || _relativeX != x) {
        _relativeX = x;
        emit relativeXChanged();
        emit changed();
    }
}

bool QDeclarativeCurve::hasRelativeX()
{
    return _relativeX.isValid();
}

qreal QDeclarativeCurve::relativeY() const
{
    return _relativeY;
}

void QDeclarativeCurve::setRelativeY(qreal y)
{
    if (_relativeY.isNull || _relativeY != y) {
        _relativeY = y;
        emit relativeYChanged();
        emit changed();
    }
}

bool QDeclarativeCurve::hasRelativeY()
{
    return _relativeY.isValid();
}

/****************************************************************************/

/*!
    \qmlclass PathAttribute QDeclarativePathAttribute
    \inqmlmodule QtQuick 2
    \ingroup qml-view-elements
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
    \snippet doc/src/snippets/declarative/pathview/pathattributes.qml 0
    (see the PathView documentation for the specification of ContactModel.qml
     used for ContactModel above.)
    \endtable


    \sa Path
*/

/*!
    \qmlproperty string QtQuick2::PathAttribute::name
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

QString QDeclarativePathAttribute::name() const
{
    return _name;
}

void QDeclarativePathAttribute::setName(const QString &name)
{
    if (_name == name)
        return;
     _name = name;
    emit nameChanged();
}

/*!
   \qmlproperty real QtQuick2::PathAttribute::value
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
qreal QDeclarativePathAttribute::value() const
{
    return _value;
}

void QDeclarativePathAttribute::setValue(qreal value)
{
    if (_value != value) {
        _value = value;
        emit valueChanged();
        emit changed();
    }
}

/****************************************************************************/

/*!
    \qmlclass PathLine QDeclarativePathLine
    \inqmlmodule QtQuick 2
    \ingroup qml-view-elements
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
    \qmlproperty real QtQuick2::PathLine::x
    \qmlproperty real QtQuick2::PathLine::y

    Defines the end point of the line.
*/

inline QPointF positionForCurve(const QDeclarativePathData &data, const QPointF &prevPoint)
{
    QDeclarativeCurve *curve = data.curves.at(data.index);
    bool isEnd = data.index == data.curves.size() - 1;
    return QPointF(curve->hasRelativeX() ? prevPoint.x() + curve->relativeX() : !isEnd || curve->hasX() ? curve->x() : data.endPoint.x(),
                   curve->hasRelativeY() ? prevPoint.y() + curve->relativeY() : !isEnd || curve->hasY() ? curve->y() : data.endPoint.y());
}

void QDeclarativePathLine::addToPath(QPainterPath &path, const QDeclarativePathData &data)
{
    path.lineTo(positionForCurve(data, path.currentPosition()));
}

/****************************************************************************/

/*!
    \qmlclass PathQuad QDeclarativePathQuad
    \inqmlmodule QtQuick 2
    \ingroup qml-view-elements
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
    \qmlproperty real QtQuick2::PathQuad::x
    \qmlproperty real QtQuick2::PathQuad::y

    Defines the end point of the curve.
*/

/*!
   \qmlproperty real QtQuick2::PathQuad::controlX
   \qmlproperty real QtQuick2::PathQuad::controlY

   Defines the position of the control point.
*/

/*!
    the x position of the control point.
*/
qreal QDeclarativePathQuad::controlX() const
{
    return _controlX;
}

void QDeclarativePathQuad::setControlX(qreal x)
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
qreal QDeclarativePathQuad::controlY() const
{
    return _controlY;
}

void QDeclarativePathQuad::setControlY(qreal y)
{
    if (_controlY != y) {
        _controlY = y;
        emit controlYChanged();
        emit changed();
    }
}

qreal QDeclarativePathQuad::relativeControlX() const
{
    return _relativeControlX;
}

void QDeclarativePathQuad::setRelativeControlX(qreal x)
{
    if (_relativeControlX.isNull || _relativeControlX != x) {
        _relativeControlX = x;
        emit relativeControlXChanged();
        emit changed();
    }
}

bool QDeclarativePathQuad::hasRelativeControlX()
{
    return _relativeControlX.isValid();
}

qreal QDeclarativePathQuad::relativeControlY() const
{
    return _relativeControlY;
}

void QDeclarativePathQuad::setRelativeControlY(qreal y)
{
    if (_relativeControlY.isNull || _relativeControlY != y) {
        _relativeControlY = y;
        emit relativeControlYChanged();
        emit changed();
    }
}

bool QDeclarativePathQuad::hasRelativeControlY()
{
    return _relativeControlY.isValid();
}

void QDeclarativePathQuad::addToPath(QPainterPath &path, const QDeclarativePathData &data)
{
    const QPointF &prevPoint = path.currentPosition();
    QPointF controlPoint(hasRelativeControlX() ? prevPoint.x() + relativeControlX() : controlX(),
                         hasRelativeControlY() ? prevPoint.y() + relativeControlY() : controlY());
    path.quadTo(controlPoint, positionForCurve(data, path.currentPosition()));
}

/****************************************************************************/

/*!
   \qmlclass PathCubic QDeclarativePathCubic
    \inqmlmodule QtQuick 2
    \ingroup qml-view-elements
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
    \qmlproperty real QtQuick2::PathCubic::x
    \qmlproperty real QtQuick2::PathCubic::y

    Defines the end point of the curve.
*/

/*!
   \qmlproperty real QtQuick2::PathCubic::control1X
   \qmlproperty real QtQuick2::PathCubic::control1Y

    Defines the position of the first control point.
*/
qreal QDeclarativePathCubic::control1X() const
{
    return _control1X;
}

void QDeclarativePathCubic::setControl1X(qreal x)
{
    if (_control1X != x) {
        _control1X = x;
        emit control1XChanged();
        emit changed();
    }
}

qreal QDeclarativePathCubic::control1Y() const
{
    return _control1Y;
}

void QDeclarativePathCubic::setControl1Y(qreal y)
{
    if (_control1Y != y) {
        _control1Y = y;
        emit control1YChanged();
        emit changed();
    }
}

/*!
   \qmlproperty real QtQuick2::PathCubic::control2X
   \qmlproperty real QtQuick2::PathCubic::control2Y

    Defines the position of the second control point.
*/
qreal QDeclarativePathCubic::control2X() const
{
    return _control2X;
}

void QDeclarativePathCubic::setControl2X(qreal x)
{
    if (_control2X != x) {
        _control2X = x;
        emit control2XChanged();
        emit changed();
    }
}

qreal QDeclarativePathCubic::control2Y() const
{
    return _control2Y;
}

void QDeclarativePathCubic::setControl2Y(qreal y)
{
    if (_control2Y != y) {
        _control2Y = y;
        emit control2YChanged();
        emit changed();
    }
}

qreal QDeclarativePathCubic::relativeControl1X() const
{
    return _relativeControl1X;
}

void QDeclarativePathCubic::setRelativeControl1X(qreal x)
{
    if (_relativeControl1X.isNull || _relativeControl1X != x) {
        _relativeControl1X = x;
        emit relativeControl1XChanged();
        emit changed();
    }
}

bool QDeclarativePathCubic::hasRelativeControl1X()
{
    return _relativeControl1X.isValid();
}

qreal QDeclarativePathCubic::relativeControl1Y() const
{
    return _relativeControl1Y;
}

void QDeclarativePathCubic::setRelativeControl1Y(qreal y)
{
    if (_relativeControl1Y.isNull || _relativeControl1Y != y) {
        _relativeControl1Y = y;
        emit relativeControl1YChanged();
        emit changed();
    }
}

bool QDeclarativePathCubic::hasRelativeControl1Y()
{
    return _relativeControl1Y.isValid();
}

qreal QDeclarativePathCubic::relativeControl2X() const
{
    return _relativeControl2X;
}

void QDeclarativePathCubic::setRelativeControl2X(qreal x)
{
    if (_relativeControl2X.isNull || _relativeControl2X != x) {
        _relativeControl2X = x;
        emit relativeControl2XChanged();
        emit changed();
    }
}

bool QDeclarativePathCubic::hasRelativeControl2X()
{
    return _relativeControl2X.isValid();
}

qreal QDeclarativePathCubic::relativeControl2Y() const
{
    return _relativeControl2Y;
}

void QDeclarativePathCubic::setRelativeControl2Y(qreal y)
{
    if (_relativeControl2Y.isNull || _relativeControl2Y != y) {
        _relativeControl2Y = y;
        emit relativeControl2YChanged();
        emit changed();
    }
}

bool QDeclarativePathCubic::hasRelativeControl2Y()
{
    return _relativeControl2Y.isValid();
}

void QDeclarativePathCubic::addToPath(QPainterPath &path, const QDeclarativePathData &data)
{
    const QPointF &prevPoint = path.currentPosition();
    QPointF controlPoint1(hasRelativeControl1X() ? prevPoint.x() + relativeControl1X() : control1X(),
                          hasRelativeControl1Y() ? prevPoint.y() + relativeControl1Y() : control1Y());
    QPointF controlPoint2(hasRelativeControl2X() ? prevPoint.x() + relativeControl2X() : control2X(),
                          hasRelativeControl2Y() ? prevPoint.y() + relativeControl2Y() : control2Y());
    path.cubicTo(controlPoint1, controlPoint2, positionForCurve(data, path.currentPosition()));
}

/****************************************************************************/

inline QPointF previousPathPosition(const QPainterPath &path)
{
    int count = path.elementCount();
    if (count < 1)
        return QPointF();

    int index = path.elementAt(count-1).type == QPainterPath::CurveToDataElement ? count - 4 : count - 2;
    return index > -1 ? QPointF(path.elementAt(index)) : path.pointAtPercent(0);
}

void QDeclarativePathCatmullRomCurve::addToPath(QPainterPath &path, const QDeclarativePathData &data)
{
    //here we convert catmull-rom spline to bezier for use in QPainterPath.
    //basic conversion algorithm:
    //  catmull-rom points * inverse bezier matrix * catmull-rom matrix = bezier points
    //each point in the catmull-rom spline produces a bezier endpoint + 2 control points
    //calculations for each point use a moving window of 4 points
    //  (previous 2 points + current point + next point)
    QPointF prevFar, prev, point, next;

    //get previous points
    int index = data.index - 1;
    QDeclarativeCurve *curve = index == -1 ? 0 : data.curves.at(index);
    if (qobject_cast<QDeclarativePathCatmullRomCurve*>(curve)) {
        prev = path.currentPosition();
        prevFar = previousPathPosition(path);
    } else
        prevFar = prev = path.currentPosition();

    //get current point
    point = positionForCurve(data, path.currentPosition());

    //get next point
    index = data.index + 1;
    if (index < data.curves.count() && qobject_cast<QDeclarativePathCatmullRomCurve*>(data.curves.at(index))) {
        QDeclarativePathData nextData;
        nextData.index = index;
        nextData.endPoint = data.endPoint;
        nextData.curves = data.curves;
        next = positionForCurve(nextData, point);
    } else
        next = point;

    /*
        full conversion matrix (inverse bezier * catmull-rom):
        0.000,  1.000,  0.000,  0.000,
        -0.167,  1.000,  0.167,  0.000,
        0.000,  0.167,  1.000, -0.167,
        0.000,  0.000,  1.000,  0.000

        conversion doesn't require full matrix multiplication,
        so below we simplify
    */
    QPointF control1(prevFar.x() * qreal(-0.167) +
                     prev.x() +
                     point.x() * qreal(0.167),
                     prevFar.y() * qreal(-0.167) +
                     prev.y() +
                     point.y() * qreal(0.167));

    QPointF control2(prev.x() * qreal(0.167) +
                     point.x() +
                     next.x() * qreal(-0.167),
                     prev.y() * qreal(0.167) +
                     point.y() +
                     next.y() * qreal(-0.167));

    path.cubicTo(control1, control2, point);
}

/****************************************************************************/

qreal QDeclarativePathArc::radiusX() const
{
    return _radiusX;
}

void QDeclarativePathArc::setRadiusX(qreal radius)
{
    if (_radiusX == radius)
        return;

    _radiusX = radius;
    emit radiusXChanged();
}

qreal QDeclarativePathArc::radiusY() const
{
    return _radiusY;
}

void QDeclarativePathArc::setRadiusY(qreal radius)
{
    if (_radiusY == radius)
        return;

    _radiusY = radius;
    emit radiusYChanged();
}

bool QDeclarativePathArc::useLargeArc() const
{
    return _useLargeArc;
}

void QDeclarativePathArc::setUseLargeArc(bool largeArc)
{
    if (_useLargeArc == largeArc)
        return;

    _useLargeArc = largeArc;
    emit useLargeArcChanged();
}

QDeclarativePathArc::ArcDirection QDeclarativePathArc::direction() const
{
    return _direction;
}

void QDeclarativePathArc::setDirection(ArcDirection direction)
{
    if (_direction == direction)
        return;

    _direction = direction;
    emit directionChanged();
}

void QDeclarativePathArc::addToPath(QPainterPath &path, const QDeclarativePathData &data)
{
    const QPointF &startPoint = path.currentPosition();
    const QPointF &endPoint = positionForCurve(data, startPoint);
    QDeclarativeSvgParser::pathArc(path,
            _radiusX,
            _radiusY,
            0,  //xAxisRotation
            _useLargeArc,
            _direction == Clockwise ? 1 : 0,
            endPoint.x(),
            endPoint.y(),
            startPoint.x(), startPoint.y());
}

/****************************************************************************/

QString QDeclarativePathSvg::path() const
{
    return _path;
}

void QDeclarativePathSvg::setPath(const QString &path)
{
    if (_path == path)
        return;

    _path = path;
    emit pathChanged();
}

void QDeclarativePathSvg::addToPath(QPainterPath &path, const QDeclarativePathData &)
{
    QDeclarativeSvgParser::parsePathDataFast(_path, path);
}

/****************************************************************************/

/*!
    \qmlclass PathPercent QDeclarativePathPercent
    \inqmlmodule QtQuick 2
    \ingroup qml-view-elements
    \brief The PathPercent manipulates the way a path is interpreted.

    PathPercent allows you to manipulate the spacing between items on a
    PathView's path. You can use it to bunch together items on part of
    the path, and spread them out on other parts of the path.

    The examples below show the normal distribution of items along a path
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
    \qmlproperty real QtQuick2::PathPercent::value
    The proportion of items that should be laid out up to this point.

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

qreal QDeclarativePathPercent::value() const
{
    return _value;
}

void QDeclarativePathPercent::setValue(qreal value)
{
    if (_value != value) {
        _value = value;
        emit valueChanged();
        emit changed();
    }
}
QT_END_NAMESPACE
