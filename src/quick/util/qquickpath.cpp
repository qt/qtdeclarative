/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQuick module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qquickpath_p.h"
#include "qquickpath_p_p.h"
#include "qquicksvgparser_p.h"

#include <QSet>
#include <QTime>

#include <private/qbezier_p.h>
#include <QtCore/qmath.h>
#include <QtCore/private/qnumeric_p.h>

QT_BEGIN_NAMESPACE

/*!
    \qmltype PathElement
    \instantiates QQuickPathElement
    \inqmlmodule QtQuick
    \ingroup qtquick-animation-paths
    \brief PathElement is the base path type.

    This type is the base for all path types.  It cannot
    be instantiated.

    \sa Path, PathAttribute, PathPercent, PathLine, PathPolyline, PathQuad, PathCubic, PathArc,
        PathAngleArc, PathCurve, PathSvg
*/

/*!
    \qmltype Path
    \instantiates QQuickPath
    \inqmlmodule QtQuick
    \ingroup qtquick-animation-paths
    \brief Defines a path for use by \l PathView and \l Shape.

    A Path is composed of one or more path segments - PathLine, PathPolyline, PathQuad,
    PathCubic, PathArc, PathAngleArc, PathCurve, PathSvg.

    The spacing of the items along the Path can be adjusted via a
    PathPercent object.

    PathAttribute allows named attributes with values to be defined
    along the path.

    Path and the other types for specifying path elements are shared between
    \l PathView and \l Shape. The following table provides an overview of the
    applicability of the various path elements:

    \table
    \header
        \li Element
        \li PathView
        \li Shape
        \li Shape, GL_NV_path_rendering
        \li Shape, software
    \row
        \li PathMove
        \li N/A
        \li Yes
        \li Yes
        \li Yes
    \row
        \li PathLine
        \li Yes
        \li Yes
        \li Yes
        \li Yes
    \row
        \li PathPolyline
        \li Yes
        \li Yes
        \li Yes
        \li Yes
    \li PathMultiLine
        \li Yes
        \li Yes
        \li Yes
        \li Yes
    \row
        \li PathQuad
        \li Yes
        \li Yes
        \li Yes
        \li Yes
    \row
        \li PathCubic
        \li Yes
        \li Yes
        \li Yes
        \li Yes
    \row
        \li PathArc
        \li Yes
        \li Yes
        \li Yes
        \li Yes
    \row
        \li PathAngleArc
        \li Yes
        \li Yes
        \li Yes
        \li Yes
    \row
        \li PathSvg
        \li Yes
        \li Yes
        \li Yes
        \li Yes
    \row
        \li PathAttribute
        \li Yes
        \li N/A
        \li N/A
        \li N/A
    \row
        \li PathPercent
        \li Yes
        \li N/A
        \li N/A
        \li N/A
    \row
        \li PathCurve
        \li Yes
        \li No
        \li No
        \li No
    \endtable

    \note Path is a non-visual type; it does not display anything on its own.
    To draw a path, use \l Shape.

    \sa PathView, Shape, PathAttribute, PathPercent, PathLine, PathPolyline, PathMove, PathQuad, PathCubic, PathArc, PathAngleArc, PathCurve, PathSvg
*/
QQuickPath::QQuickPath(QObject *parent)
 : QObject(*(new QQuickPathPrivate), parent)
{
}

QQuickPath::QQuickPath(QQuickPathPrivate &dd, QObject *parent)
    : QObject(dd, parent)
{
}

QQuickPath::~QQuickPath()
{
}

/*!
    \qmlproperty real QtQuick::Path::startX
    \qmlproperty real QtQuick::Path::startY
    These properties hold the starting position of the path.
*/
qreal QQuickPath::startX() const
{
    Q_D(const QQuickPath);
    return d->startX.isNull ? 0 : d->startX.value;
}

void QQuickPath::setStartX(qreal x)
{
    Q_D(QQuickPath);
    if (d->startX.isValid() && qFuzzyCompare(x, d->startX))
        return;
    d->startX = x;
    emit startXChanged();
    processPath();
}

bool QQuickPath::hasStartX() const
{
    Q_D(const QQuickPath);
    return d->startX.isValid();
}

qreal QQuickPath::startY() const
{
    Q_D(const QQuickPath);
    return d->startY.isNull ? 0 : d->startY.value;
}

void QQuickPath::setStartY(qreal y)
{
    Q_D(QQuickPath);
    if (d->startY.isValid() && qFuzzyCompare(y, d->startY))
        return;
    d->startY = y;
    emit startYChanged();
    processPath();
}

bool QQuickPath::hasStartY() const
{
    Q_D(const QQuickPath);
    return d->startY.isValid();
}

/*!
    \qmlproperty bool QtQuick::Path::closed
    This property holds whether the start and end of the path are identical.
*/
bool QQuickPath::isClosed() const
{
    Q_D(const QQuickPath);
    return d->closed;
}

/*!
    \qmlproperty list<PathElement> QtQuick::Path::pathElements
    This property holds the objects composing the path.

    \default

    A path can contain the following path objects:
    \list
        \li \l PathLine - a straight line to a given position.
        \li \l PathPolyline - a polyline specified as a list of coordinates.
        \li \l PathMultiline - a list of polylines specified as a list of lists of coordinates.
        \li \l PathQuad - a quadratic Bezier curve to a given position with a control point.
        \li \l PathCubic - a cubic Bezier curve to a given position with two control points.
        \li \l PathArc - an arc to a given position with a radius.
        \li \l PathAngleArc - an arc specified by center point, radii, and angles.
        \li \l PathSvg - a path specified as an SVG path data string.
        \li \l PathCurve - a point on a Catmull-Rom curve.
        \li \l PathAttribute - an attribute at a given position in the path.
        \li \l PathPercent - a way to spread out items along various segments of the path.
    \endlist

    \snippet qml/pathview/pathattributes.qml 2
*/

QQmlListProperty<QQuickPathElement> QQuickPath::pathElements()
{
    return QQmlListProperty<QQuickPathElement>(this,
                                               nullptr,
                                               pathElements_append,
                                               pathElements_count,
                                               pathElements_at,
                                               pathElements_clear);
}

static QQuickPathPrivate *privatePath(QObject *object)
{
    QQuickPath *path = static_cast<QQuickPath*>(object);

    return QQuickPathPrivate::get(path);
}

QQuickPathElement *QQuickPath::pathElements_at(QQmlListProperty<QQuickPathElement> *property, int index)
{
    QQuickPathPrivate *d = privatePath(property->object);

    return d->_pathElements.at(index);
}

void QQuickPath::pathElements_append(QQmlListProperty<QQuickPathElement> *property, QQuickPathElement *pathElement)
{
    QQuickPathPrivate *d = privatePath(property->object);
    QQuickPath *path = static_cast<QQuickPath*>(property->object);

    d->_pathElements.append(pathElement);

    if (d->componentComplete) {
        QQuickCurve *curve = qobject_cast<QQuickCurve *>(pathElement);
        if (curve)
            d->_pathCurves.append(curve);
        else if (QQuickPathText *text = qobject_cast<QQuickPathText *>(pathElement))
            d->_pathTexts.append(text);
        else {
            QQuickPathAttribute *attribute = qobject_cast<QQuickPathAttribute *>(pathElement);
            if (attribute && !d->_attributes.contains(attribute->name()))
                d->_attributes.append(attribute->name());
        }

        path->processPath();

        connect(pathElement, SIGNAL(changed()), path, SLOT(processPath()));
    }
}

int QQuickPath::pathElements_count(QQmlListProperty<QQuickPathElement> *property)
{
    QQuickPathPrivate *d = privatePath(property->object);

    return d->_pathElements.count();
}

void QQuickPath::pathElements_clear(QQmlListProperty<QQuickPathElement> *property)
{
    QQuickPathPrivate *d = privatePath(property->object);
    QQuickPath *path = static_cast<QQuickPath*>(property->object);

    path->disconnectPathElements();
    d->_pathElements.clear();
    d->_pathCurves.clear();
    d->_pointCache.clear();
    d->_pathTexts.clear();
}

void QQuickPath::interpolate(int idx, const QString &name, qreal value)
{
    Q_D(QQuickPath);
    interpolate(d->_attributePoints, idx, name, value);
}

void QQuickPath::interpolate(QList<AttributePoint> &attributePoints, int idx, const QString &name, qreal value)
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

void QQuickPath::endpoint(const QString &name)
{
    Q_D(QQuickPath);
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

void QQuickPath::endpoint(QList<AttributePoint> &attributePoints, const QString &name)
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

void QQuickPath::processPath()
{
    Q_D(QQuickPath);

    if (!d->componentComplete)
        return;

    d->_pointCache.clear();
    d->prevBez.isValid = false;

    if (d->isShapePath) {
        // This path is a ShapePath, so avoid extra overhead
        d->_path = createShapePath(QPointF(), QPointF(), d->pathLength, &d->closed);
    } else {
        d->_path = createPath(QPointF(), QPointF(), d->_attributes, d->pathLength, d->_attributePoints, &d->closed);
    }

    emit changed();
}

inline static void scalePath(QPainterPath &path, const QSizeF &scale)
{
    const qreal xscale = scale.width();
    const qreal yscale = scale.height();
    if (xscale == 1 && yscale ==  1)
        return;

    for (int i = 0; i < path.elementCount(); ++i) {
        const QPainterPath::Element &element = path.elementAt(i);
        path.setElementPositionAt(i, element.x * xscale, element.y * yscale);
    }
}

QPainterPath QQuickPath::createPath(const QPointF &startPoint, const QPointF &endPoint, const QStringList &attributes, qreal &pathLength, QList<AttributePoint> &attributePoints, bool *closed)
{
    Q_D(QQuickPath);

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

    const QString percentString = QStringLiteral("_qfx_percent");

    bool usesPercent = false;
    int index = 0;
    for (QQuickPathElement *pathElement : qAsConst(d->_pathElements)) {
        if (QQuickCurve *curve = qobject_cast<QQuickCurve *>(pathElement)) {
            QQuickPathData data;
            data.index = index;
            data.endPoint = endPoint;
            data.curves = d->_pathCurves;
            curve->addToPath(path, data);
            AttributePoint p;
            p.origpercent = path.length();
            attributePoints << p;
            ++index;
        } else if (QQuickPathAttribute *attribute = qobject_cast<QQuickPathAttribute *>(pathElement)) {
            AttributePoint &point = attributePoints.last();
            point.values[attribute->name()] = attribute->value();
            interpolate(attributePoints, attributePoints.count() - 1, attribute->name(), attribute->value());
        } else if (QQuickPathPercent *percent = qobject_cast<QQuickPathPercent *>(pathElement)) {
            AttributePoint &point = attributePoints.last();
            point.values[percentString] = percent->value();
            interpolate(attributePoints, attributePoints.count() - 1, percentString, percent->value());
            usesPercent = true;
        } else if (QQuickPathText *text = qobject_cast<QQuickPathText *>(pathElement)) {
            text->addToPath(path);
        }
    }

    // Fixup end points
    const AttributePoint &last = attributePoints.constLast();
    for (int ii = 0; ii < attributes.count(); ++ii) {
        if (!last.values.contains(attributes.at(ii)))
            endpoint(attributePoints, attributes.at(ii));
    }
    if (usesPercent && !last.values.contains(percentString)) {
        d->_attributePoints.last().values[percentString] = 1;
        interpolate(d->_attributePoints.count() - 1, percentString, 1);
    }
    scalePath(path, d->scale);

    // Adjust percent
    qreal length = path.length();
    qreal prevpercent = 0;
    qreal prevorigpercent = 0;
    for (int ii = 0; ii < attributePoints.count(); ++ii) {
        const AttributePoint &point = attributePoints.at(ii);
        if (point.values.contains(percentString)) { //special string for QQuickPathPercent
            if ( ii > 0) {
                qreal scale = (attributePoints[ii].origpercent/length - prevorigpercent) /
                            (point.values.value(percentString)-prevpercent);
                attributePoints[ii].scale = scale;
            }
            attributePoints[ii].origpercent /= length;
            attributePoints[ii].percent = point.values.value(percentString);
            prevorigpercent = attributePoints.at(ii).origpercent;
            prevpercent = attributePoints.at(ii).percent;
        } else {
            attributePoints[ii].origpercent /= length;
            attributePoints[ii].percent = attributePoints.at(ii).origpercent;
        }
    }

    if (closed) {
        QPointF end = path.currentPosition();
        *closed = length > 0 && startX * d->scale.width() == end.x() && startY * d->scale.height() == end.y();
    }
    pathLength = length;

    return path;
}

QPainterPath QQuickPath::createShapePath(const QPointF &startPoint, const QPointF &endPoint, qreal &pathLength, bool *closed)
{
    Q_D(QQuickPath);

    if (!d->componentComplete)
        return QPainterPath();

    QPainterPath path;

    qreal startX = d->startX.isValid() ? d->startX.value : startPoint.x();
    qreal startY = d->startY.isValid() ? d->startY.value : startPoint.y();
    path.moveTo(startX, startY);

    int index = 0;
    for (QQuickCurve *curve : qAsConst(d->_pathCurves)) {
        QQuickPathData data;
        data.index = index;
        data.endPoint = endPoint;
        data.curves = d->_pathCurves;
        curve->addToPath(path, data);
        ++index;
    }

    for (QQuickPathText *text : qAsConst(d->_pathTexts))
        text->addToPath(path);

    if (closed) {
        QPointF end = path.currentPosition();
        *closed = startX == end.x() && startY == end.y();
    }
    scalePath(path, d->scale);

    // Note: Length of paths inside ShapePath is not used, so currently
    // length is always 0. This avoids potentially heavy path.length()
    //pathLength = path.length();
    pathLength = 0;

    return path;
}

void QQuickPath::classBegin()
{
    Q_D(QQuickPath);
    d->componentComplete = false;
}

void QQuickPath::disconnectPathElements()
{
    Q_D(const QQuickPath);

    for (QQuickPathElement *pathElement : d->_pathElements)
        disconnect(pathElement, SIGNAL(changed()), this, SLOT(processPath()));
}

void QQuickPath::connectPathElements()
{
    Q_D(const QQuickPath);

    for (QQuickPathElement *pathElement : d->_pathElements)
        connect(pathElement, SIGNAL(changed()), this, SLOT(processPath()));
}

void QQuickPath::gatherAttributes()
{
    Q_D(QQuickPath);

    QSet<QString> attributes;

    // First gather up all the attributes
    for (QQuickPathElement *pathElement : qAsConst(d->_pathElements)) {
        if (QQuickCurve *curve = qobject_cast<QQuickCurve *>(pathElement))
            d->_pathCurves.append(curve);
        else if (QQuickPathText *text = qobject_cast<QQuickPathText *>(pathElement))
            d->_pathTexts.append(text);
        else if (QQuickPathAttribute *attribute = qobject_cast<QQuickPathAttribute *>(pathElement))
            attributes.insert(attribute->name());
    }

    d->_attributes = attributes.values();
}

void QQuickPath::componentComplete()
{
    Q_D(QQuickPath);
    d->componentComplete = true;

    gatherAttributes();

    processPath();

    connectPathElements();
}

QPainterPath QQuickPath::path() const
{
    Q_D(const QQuickPath);
    return d->_path;
}

QStringList QQuickPath::attributes() const
{
    Q_D(const QQuickPath);
    if (!d->componentComplete) {
        QSet<QString> attrs;

        // First gather up all the attributes
        for (QQuickPathElement *pathElement : d->_pathElements) {
            if (QQuickPathAttribute *attribute =
                qobject_cast<QQuickPathAttribute *>(pathElement))
                attrs.insert(attribute->name());
        }
        return attrs.values();
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

static inline int segmentCount(const QPainterPath &path, qreal pathLength)
{
    // In the really simple case of a single straight line we can interpolate without jitter
    // between just two points.
    if (path.elementCount() == 2
            && path.elementAt(0).type == QPainterPath::MoveToElement
            && path.elementAt(1).type == QPainterPath::LineToElement) {
        return 1;
    }
    // more points means less jitter between items as they move along the
    // path, but takes longer to generate
    return qCeil(pathLength*5);
}

//derivative of the equation
static inline qreal slopeAt(qreal t, qreal a, qreal b, qreal c, qreal d)
{
    return 3*t*t*(d - 3*c + 3*b - a) + 6*t*(c - 2*b + a) + 3*(b - a);
}

void QQuickPath::createPointCache() const
{
    Q_D(const QQuickPath);
    qreal pathLength = d->pathLength;
    if (pathLength <= 0 || qt_is_nan(pathLength))
        return;

    const int segments = segmentCount(d->_path, pathLength);
    const int lastElement = d->_path.elementCount() - 1;
    d->_pointCache.resize(segments+1);

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
            qreal percent = qreal(i)/segments;
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

void QQuickPath::invalidateSequentialHistory() const
{
    Q_D(const QQuickPath);
    d->prevBez.isValid = false;
}

/*!
    \qmlproperty size QtQuick::Path::scale

    This property holds the scale factor for the path.
    The width and height of \a scale can be different, to
    achieve anisotropic scaling.

    \note Setting this property will not affect the border width.

    \since QtQuick 2.14
*/
QSizeF QQuickPath::scale() const
{
    Q_D(const QQuickPath);
    return d->scale;
}

void QQuickPath::setScale(const QSizeF &scale)
{
    Q_D(QQuickPath);
    if (scale == d->scale)
        return;
    d->scale = scale;
    emit scaleChanged();
    processPath();
}

QPointF QQuickPath::sequentialPointAt(qreal p, qreal *angle) const
{
    Q_D(const QQuickPath);
    return sequentialPointAt(d->_path, d->pathLength, d->_attributePoints, d->prevBez, p, angle);
}

QPointF QQuickPath::sequentialPointAt(const QPainterPath &path, const qreal &pathLength, const QList<AttributePoint> &attributePoints, QQuickCachedBezier &prevBez, qreal p, qreal *angle)
{
    Q_ASSERT(p >= 0.0 && p <= 1.0);

    if (!prevBez.isValid)
        return p > .5 ? backwardsPointAt(path, pathLength, attributePoints, prevBez, p, angle) :
                        forwardsPointAt(path, pathLength, attributePoints, prevBez, p, angle);

    return p < prevBez.p ? backwardsPointAt(path, pathLength, attributePoints, prevBez, p, angle) :
                           forwardsPointAt(path, pathLength, attributePoints, prevBez, p, angle);
}

QPointF QQuickPath::forwardsPointAt(const QPainterPath &path, const qreal &pathLength, const QList<AttributePoint> &attributePoints, QQuickCachedBezier &prevBez, qreal p, qreal *angle)
{
    if (pathLength <= 0 || qt_is_nan(pathLength))
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
                Q_UNUSED(lastElement);
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
QPointF QQuickPath::backwardsPointAt(const QPainterPath &path, const qreal &pathLength, const QList<AttributePoint> &attributePoints, QQuickCachedBezier &prevBez, qreal p, qreal *angle)
{
    if (pathLength <= 0 || qt_is_nan(pathLength))
        return path.pointAtPercent(0);

    const int firstElement = 1; //element 0 is always a MoveTo, which we ignore
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
                Q_UNUSED(firstElement);
                currBez = nextBezier(path, &currElement, &bezLength, true /*reverse*/);
                //special case for first element is to avoid floating point math
                //causing an epc that never hits 0.
                currLength = (currElement == firstElement) ? bezLength : prevLength;
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

/*!
   \qmlmethod point Path::pointAtPercent(real t)

    Returns the point at the percentage \a t of the current path.
    The argument \a t has to be between 0 and 1.

    \note Similarly to other percent methods in \l QPainterPath,
    the percentage measurement is not linear with regards to the length,
    if curves are present in the path.
    When curves are present, the percentage argument is mapped to the \c t
    parameter of the Bezier equations.

   \sa QPainterPath::pointAtPercent()

   \since QtQuick 2.14
*/
QPointF QQuickPath::pointAtPercent(qreal t) const
{
    Q_D(const QQuickPath);
    if (d->isShapePath)                     // this since ShapePath does not calculate the length at all,
        return d->_path.pointAtPercent(t);  // in order to be faster.

    if (d->_pointCache.isEmpty()) {
        createPointCache();
        if (d->_pointCache.isEmpty())
            return QPointF();
    }

    const int segmentCount = d->_pointCache.size() - 1;
    qreal idxf = t*segmentCount;
    int idx1 = qFloor(idxf);
    qreal delta = idxf - idx1;
    if (idx1 > segmentCount)
        idx1 = segmentCount;
    else if (idx1 < 0)
        idx1 = 0;

    if (delta == 0.0)
        return d->_pointCache.at(idx1);

    // interpolate between the two points.
    int idx2 = qCeil(idxf);
    if (idx2 > segmentCount)
        idx2 = segmentCount;
    else if (idx2 < 0)
        idx2 = 0;

    QPointF p1 = d->_pointCache.at(idx1);
    QPointF p2 = d->_pointCache.at(idx2);
    QPointF pos = p1 * (1.0-delta) + p2 * delta;

    return pos;
}

qreal QQuickPath::attributeAt(const QString &name, qreal percent) const
{
    Q_D(const QQuickPath);
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

qreal QQuickCurve::x() const
{
    return _x.isNull ? 0 : _x.value;
}

void QQuickCurve::setX(qreal x)
{
    if (_x.isNull || _x != x) {
        _x = x;
        emit xChanged();
        emit changed();
    }
}

bool QQuickCurve::hasX()
{
    return _x.isValid();
}

qreal QQuickCurve::y() const
{
    return _y.isNull ? 0 : _y.value;
}

void QQuickCurve::setY(qreal y)
{
    if (_y.isNull || _y != y) {
        _y = y;
        emit yChanged();
        emit changed();
    }
}

bool QQuickCurve::hasY()
{
    return _y.isValid();
}

qreal QQuickCurve::relativeX() const
{
    return _relativeX;
}

void QQuickCurve::setRelativeX(qreal x)
{
    if (_relativeX.isNull || _relativeX != x) {
        _relativeX = x;
        emit relativeXChanged();
        emit changed();
    }
}

bool QQuickCurve::hasRelativeX()
{
    return _relativeX.isValid();
}

qreal QQuickCurve::relativeY() const
{
    return _relativeY;
}

void QQuickCurve::setRelativeY(qreal y)
{
    if (_relativeY.isNull || _relativeY != y) {
        _relativeY = y;
        emit relativeYChanged();
        emit changed();
    }
}

bool QQuickCurve::hasRelativeY()
{
    return _relativeY.isValid();
}

/****************************************************************************/

/*!
    \qmltype PathAttribute
    \instantiates QQuickPathAttribute
    \inqmlmodule QtQuick
    \ingroup qtquick-animation-paths
    \brief Specifies how to set an attribute at a given position in a Path.

    The PathAttribute object allows attributes consisting of a name and
    a value to be specified for various points along a path.  The
    attributes are exposed to the delegate as
    \l{Attached Properties and Attached Signal Handlers} {Attached Properties}.
    The value of an attribute at any particular point along the path is interpolated
    from the PathAttributes bounding that point.

    The example below shows a path with the items scaled to 30% with
    opacity 50% at the top of the path and scaled 100% with opacity
    100% at the bottom.  Note the use of the PathView.iconScale and
    PathView.iconOpacity attached properties to set the scale and opacity
    of the delegate.

    \table
    \row
    \li \image declarative-pathattribute.png
    \li
    \snippet qml/pathview/pathattributes.qml 0
    (see the PathView documentation for the specification of ContactModel.qml
     used for ContactModel above.)
    \endtable


    \sa Path
*/

/*!
    \qmlproperty string QtQuick::PathAttribute::name
    This property holds the name of the attribute to change.

    This attribute will be available to the delegate as PathView.<name>

    Note that using an existing Item property name such as "opacity" as an
    attribute is allowed.  This is because path attributes add a new
    \l{Attached Properties and Attached Signal Handlers} {Attached Property}
    which in no way clashes with existing properties.
*/

/*!
    the name of the attribute to change.
*/

QString QQuickPathAttribute::name() const
{
    return _name;
}

void QQuickPathAttribute::setName(const QString &name)
{
    if (_name == name)
        return;
     _name = name;
    emit nameChanged();
}

/*!
   \qmlproperty real QtQuick::PathAttribute::value
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
   \l{Attached Properties and Attached Signal Handlers} {Attached Property}
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
qreal QQuickPathAttribute::value() const
{
    return _value;
}

void QQuickPathAttribute::setValue(qreal value)
{
    if (_value != value) {
        _value = value;
        emit valueChanged();
        emit changed();
    }
}

/****************************************************************************/

/*!
    \qmltype PathLine
    \instantiates QQuickPathLine
    \inqmlmodule QtQuick
    \ingroup qtquick-animation-paths
    \brief Defines a straight line.

    The example below creates a path consisting of a straight line from
    0,100 to 200,100:

    \qml
    Path {
        startX: 0; startY: 100
        PathLine { x: 200; y: 100 }
    }
    \endqml

    \sa Path, PathQuad, PathCubic, PathArc, PathAngleArc, PathCurve, PathSvg, PathMove, PathPolyline
*/

/*!
    \qmlproperty real QtQuick::PathLine::x
    \qmlproperty real QtQuick::PathLine::y

    Defines the end point of the line.

    \sa relativeX, relativeY
*/

/*!
    \qmlproperty real QtQuick::PathLine::relativeX
    \qmlproperty real QtQuick::PathLine::relativeY

    Defines the end point of the line relative to its start.

    If both a relative and absolute end position are specified for a single axis, the relative
    position will be used.

    Relative and absolute positions can be mixed, for example it is valid to set a relative x
    and an absolute y.

    \sa x, y
*/

inline QPointF positionForCurve(const QQuickPathData &data, const QPointF &prevPoint)
{
    QQuickCurve *curve = data.curves.at(data.index);
    bool isEnd = data.index == data.curves.size() - 1;
    return QPointF(curve->hasRelativeX() ? prevPoint.x() + curve->relativeX() : !isEnd || curve->hasX() ? curve->x() : data.endPoint.x(),
                   curve->hasRelativeY() ? prevPoint.y() + curve->relativeY() : !isEnd || curve->hasY() ? curve->y() : data.endPoint.y());
}

void QQuickPathLine::addToPath(QPainterPath &path, const QQuickPathData &data)
{
    path.lineTo(positionForCurve(data, path.currentPosition()));
}

/****************************************************************************/

/*!
    \qmltype PathMove
    \instantiates QQuickPathMove
    \inqmlmodule QtQuick
    \ingroup qtquick-animation-paths
    \brief Moves the Path's position.

    The example below creates a path consisting of two horizontal lines with
    some empty space between them. All three segments have a width of 100:

    \qml
    Path {
        startX: 0; startY: 100
        PathLine { relativeX: 100; y: 100 }
        PathMove { relativeX: 100; y: 100 }
        PathLine { relativeX: 100; y: 100 }
    }
    \endqml

    \note PathMove should not be used in a Path associated with a PathView. Use
    PathLine instead. For ShapePath however it is important to distinguish
    between the operations of drawing a straight line and moving the path
    position without drawing anything.

    \sa Path, PathQuad, PathCubic, PathArc, PathAngleArc, PathCurve, PathSvg, PathLine
*/

/*!
    \qmlproperty real QtQuick::PathMove::x
    \qmlproperty real QtQuick::PathMove::y

    Defines the position to move to.

    \sa relativeX, relativeY
*/

/*!
    \qmlproperty real QtQuick::PathMove::relativeX
    \qmlproperty real QtQuick::PathMove::relativeY

    Defines the position to move to relative to its start.

    If both a relative and absolute end position are specified for a single axis, the relative
    position will be used.

    Relative and absolute positions can be mixed, for example it is valid to set a relative x
    and an absolute y.

    \sa x, y
*/

void QQuickPathMove::addToPath(QPainterPath &path, const QQuickPathData &data)
{
    path.moveTo(positionForCurve(data, path.currentPosition()));
}

/****************************************************************************/

/*!
    \qmltype PathQuad
    \instantiates QQuickPathQuad
    \inqmlmodule QtQuick
    \ingroup qtquick-animation-paths
    \brief Defines a quadratic Bezier curve with a control point.

    The following QML produces the path shown below:
    \table
    \row
    \li \image declarative-pathquad.png
    \li
    \qml
    Path {
        startX: 0; startY: 0
        PathQuad { x: 200; y: 0; controlX: 100; controlY: 150 }
    }
    \endqml
    \endtable

    \sa Path, PathCubic, PathLine, PathArc, PathAngleArc, PathCurve, PathSvg
*/

/*!
    \qmlproperty real QtQuick::PathQuad::x
    \qmlproperty real QtQuick::PathQuad::y

    Defines the end point of the curve.

    \sa relativeX, relativeY
*/

/*!
    \qmlproperty real QtQuick::PathQuad::relativeX
    \qmlproperty real QtQuick::PathQuad::relativeY

    Defines the end point of the curve relative to its start.

    If both a relative and absolute end position are specified for a single axis, the relative
    position will be used.

    Relative and absolute positions can be mixed, for example it is valid to set a relative x
    and an absolute y.

    \sa x, y
*/

/*!
   \qmlproperty real QtQuick::PathQuad::controlX
   \qmlproperty real QtQuick::PathQuad::controlY

   Defines the position of the control point.
*/

/*!
    the x position of the control point.
*/
qreal QQuickPathQuad::controlX() const
{
    return _controlX;
}

void QQuickPathQuad::setControlX(qreal x)
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
qreal QQuickPathQuad::controlY() const
{
    return _controlY;
}

void QQuickPathQuad::setControlY(qreal y)
{
    if (_controlY != y) {
        _controlY = y;
        emit controlYChanged();
        emit changed();
    }
}

/*!
   \qmlproperty real QtQuick::PathQuad::relativeControlX
   \qmlproperty real QtQuick::PathQuad::relativeControlY

    Defines the position of the control point relative to the curve's start.

    If both a relative and absolute control position are specified for a single axis, the relative
    position will be used.

    Relative and absolute positions can be mixed, for example it is valid to set a relative control x
    and an absolute control y.

    \sa controlX, controlY
*/

qreal QQuickPathQuad::relativeControlX() const
{
    return _relativeControlX;
}

void QQuickPathQuad::setRelativeControlX(qreal x)
{
    if (_relativeControlX.isNull || _relativeControlX != x) {
        _relativeControlX = x;
        emit relativeControlXChanged();
        emit changed();
    }
}

bool QQuickPathQuad::hasRelativeControlX()
{
    return _relativeControlX.isValid();
}

qreal QQuickPathQuad::relativeControlY() const
{
    return _relativeControlY;
}

void QQuickPathQuad::setRelativeControlY(qreal y)
{
    if (_relativeControlY.isNull || _relativeControlY != y) {
        _relativeControlY = y;
        emit relativeControlYChanged();
        emit changed();
    }
}

bool QQuickPathQuad::hasRelativeControlY()
{
    return _relativeControlY.isValid();
}

void QQuickPathQuad::addToPath(QPainterPath &path, const QQuickPathData &data)
{
    const QPointF &prevPoint = path.currentPosition();
    QPointF controlPoint(hasRelativeControlX() ? prevPoint.x() + relativeControlX() : controlX(),
                         hasRelativeControlY() ? prevPoint.y() + relativeControlY() : controlY());
    path.quadTo(controlPoint, positionForCurve(data, path.currentPosition()));
}

/****************************************************************************/

/*!
    \qmltype PathCubic
    \instantiates QQuickPathCubic
    \inqmlmodule QtQuick
    \ingroup qtquick-animation-paths
    \brief Defines a cubic Bezier curve with two control points.

    The following QML produces the path shown below:
    \table
    \row
    \li \image declarative-pathcubic.png
    \li
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

    \sa Path, PathQuad, PathLine, PathArc, PathAngleArc, PathCurve, PathSvg
*/

/*!
    \qmlproperty real QtQuick::PathCubic::x
    \qmlproperty real QtQuick::PathCubic::y

    Defines the end point of the curve.

    \sa relativeX, relativeY
*/

/*!
    \qmlproperty real QtQuick::PathCubic::relativeX
    \qmlproperty real QtQuick::PathCubic::relativeY

    Defines the end point of the curve relative to its start.

    If both a relative and absolute end position are specified for a single axis, the relative
    position will be used.

    Relative and absolute positions can be mixed, for example it is valid to set a relative x
    and an absolute y.

    \sa x, y
*/

/*!
   \qmlproperty real QtQuick::PathCubic::control1X
   \qmlproperty real QtQuick::PathCubic::control1Y

    Defines the position of the first control point.
*/
qreal QQuickPathCubic::control1X() const
{
    return _control1X;
}

void QQuickPathCubic::setControl1X(qreal x)
{
    if (_control1X != x) {
        _control1X = x;
        emit control1XChanged();
        emit changed();
    }
}

qreal QQuickPathCubic::control1Y() const
{
    return _control1Y;
}

void QQuickPathCubic::setControl1Y(qreal y)
{
    if (_control1Y != y) {
        _control1Y = y;
        emit control1YChanged();
        emit changed();
    }
}

/*!
   \qmlproperty real QtQuick::PathCubic::control2X
   \qmlproperty real QtQuick::PathCubic::control2Y

    Defines the position of the second control point.
*/
qreal QQuickPathCubic::control2X() const
{
    return _control2X;
}

void QQuickPathCubic::setControl2X(qreal x)
{
    if (_control2X != x) {
        _control2X = x;
        emit control2XChanged();
        emit changed();
    }
}

qreal QQuickPathCubic::control2Y() const
{
    return _control2Y;
}

void QQuickPathCubic::setControl2Y(qreal y)
{
    if (_control2Y != y) {
        _control2Y = y;
        emit control2YChanged();
        emit changed();
    }
}

/*!
   \qmlproperty real QtQuick::PathCubic::relativeControl1X
   \qmlproperty real QtQuick::PathCubic::relativeControl1Y
   \qmlproperty real QtQuick::PathCubic::relativeControl2X
   \qmlproperty real QtQuick::PathCubic::relativeControl2Y

    Defines the positions of the control points relative to the curve's start.

    If both a relative and absolute control position are specified for a control point's axis, the relative
    position will be used.

    Relative and absolute positions can be mixed, for example it is valid to set a relative control1 x
    and an absolute control1 y.

    \sa control1X, control1Y, control2X, control2Y
*/

qreal QQuickPathCubic::relativeControl1X() const
{
    return _relativeControl1X;
}

void QQuickPathCubic::setRelativeControl1X(qreal x)
{
    if (_relativeControl1X.isNull || _relativeControl1X != x) {
        _relativeControl1X = x;
        emit relativeControl1XChanged();
        emit changed();
    }
}

bool QQuickPathCubic::hasRelativeControl1X()
{
    return _relativeControl1X.isValid();
}

qreal QQuickPathCubic::relativeControl1Y() const
{
    return _relativeControl1Y;
}

void QQuickPathCubic::setRelativeControl1Y(qreal y)
{
    if (_relativeControl1Y.isNull || _relativeControl1Y != y) {
        _relativeControl1Y = y;
        emit relativeControl1YChanged();
        emit changed();
    }
}

bool QQuickPathCubic::hasRelativeControl1Y()
{
    return _relativeControl1Y.isValid();
}

qreal QQuickPathCubic::relativeControl2X() const
{
    return _relativeControl2X;
}

void QQuickPathCubic::setRelativeControl2X(qreal x)
{
    if (_relativeControl2X.isNull || _relativeControl2X != x) {
        _relativeControl2X = x;
        emit relativeControl2XChanged();
        emit changed();
    }
}

bool QQuickPathCubic::hasRelativeControl2X()
{
    return _relativeControl2X.isValid();
}

qreal QQuickPathCubic::relativeControl2Y() const
{
    return _relativeControl2Y;
}

void QQuickPathCubic::setRelativeControl2Y(qreal y)
{
    if (_relativeControl2Y.isNull || _relativeControl2Y != y) {
        _relativeControl2Y = y;
        emit relativeControl2YChanged();
        emit changed();
    }
}

bool QQuickPathCubic::hasRelativeControl2Y()
{
    return _relativeControl2Y.isValid();
}

void QQuickPathCubic::addToPath(QPainterPath &path, const QQuickPathData &data)
{
    const QPointF &prevPoint = path.currentPosition();
    QPointF controlPoint1(hasRelativeControl1X() ? prevPoint.x() + relativeControl1X() : control1X(),
                          hasRelativeControl1Y() ? prevPoint.y() + relativeControl1Y() : control1Y());
    QPointF controlPoint2(hasRelativeControl2X() ? prevPoint.x() + relativeControl2X() : control2X(),
                          hasRelativeControl2Y() ? prevPoint.y() + relativeControl2Y() : control2Y());
    path.cubicTo(controlPoint1, controlPoint2, positionForCurve(data, path.currentPosition()));
}

/****************************************************************************/

/*!
    \qmltype PathCurve
    \instantiates QQuickPathCatmullRomCurve
    \inqmlmodule QtQuick
    \ingroup qtquick-animation-paths
    \brief Defines a point on a Catmull-Rom curve.

    PathCurve provides an easy way to specify a curve passing directly through a set of points.
    Typically multiple PathCurves are used in a series, as the following example demonstrates:

    \snippet qml/path/basiccurve.qml 0

    This example produces the following path (with the starting point and PathCurve points
    highlighted in red):

    \image declarative-pathcurve.png

    \sa Path, PathLine, PathQuad, PathCubic, PathArc, PathSvg
*/

/*!
    \qmlproperty real QtQuick::PathCurve::x
    \qmlproperty real QtQuick::PathCurve::y

    Defines the end point of the curve.

    \sa relativeX, relativeY
*/

/*!
    \qmlproperty real QtQuick::PathCurve::relativeX
    \qmlproperty real QtQuick::PathCurve::relativeY

    Defines the end point of the curve relative to its start.

    If both a relative and absolute end position are specified for a single axis, the relative
    position will be used.

    Relative and absolute positions can be mixed, for example it is valid to set a relative x
    and an absolute y.

    \sa x, y
*/

inline QPointF previousPathPosition(const QPainterPath &path)
{
    int count = path.elementCount();
    if (count < 1)
        return QPointF();

    int index = path.elementAt(count-1).type == QPainterPath::CurveToDataElement ? count - 4 : count - 2;
    return index > -1 ? QPointF(path.elementAt(index)) : path.pointAtPercent(0);
}

void QQuickPathCatmullRomCurve::addToPath(QPainterPath &path, const QQuickPathData &data)
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
    QQuickCurve *curve = index == -1 ? 0 : data.curves.at(index);
    if (qobject_cast<QQuickPathCatmullRomCurve*>(curve)) {
        prev = path.currentPosition();
        prevFar = previousPathPosition(path);
    } else {
        prev = path.currentPosition();
        bool prevFarSet = false;
        if (index == -1 && data.curves.count() > 1) {
            if (qobject_cast<QQuickPathCatmullRomCurve*>(data.curves.at(data.curves.count()-1))) {
                //TODO: profile and optimize
                QPointF pos = prev;
                QQuickPathData loopData;
                loopData.endPoint = data.endPoint;
                loopData.curves = data.curves;
                for (int i = data.index; i < data.curves.count(); ++i) {
                    loopData.index = i;
                    pos = positionForCurve(loopData, pos);
                    if (i == data.curves.count()-2)
                        prevFar = pos;
                }
                if (pos == QPointF(path.elementAt(0))) {
                    //this is a closed path starting and ending with catmull-rom segments.
                    //we try to smooth the join point
                    prevFarSet = true;
                }
            }
        }
        if (!prevFarSet)
            prevFar = prev;
    }

    //get current point
    point = positionForCurve(data, path.currentPosition());

    //get next point
    index = data.index + 1;
    if (index < data.curves.count() && qobject_cast<QQuickPathCatmullRomCurve*>(data.curves.at(index))) {
        QQuickPathData nextData;
        nextData.index = index;
        nextData.endPoint = data.endPoint;
        nextData.curves = data.curves;
        next = positionForCurve(nextData, point);
    } else {
        if (point == QPointF(path.elementAt(0)) && qobject_cast<QQuickPathCatmullRomCurve*>(data.curves.at(0)) && path.elementCount() >= 3) {
            //this is a closed path starting and ending with catmull-rom segments.
            //we try to smooth the join point
            next = QPointF(path.elementAt(3));  //the first catmull-rom point
        } else
            next = point;
    }

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

/*!
    \qmltype PathArc
    \instantiates QQuickPathArc
    \inqmlmodule QtQuick
    \ingroup qtquick-animation-paths
    \brief Defines an arc with the given radius.

    PathArc provides a simple way of specifying an arc that ends at a given position
    and uses the specified radius. It is modeled after the SVG elliptical arc command.

    The following QML produces the path shown below:
    \table
    \row
    \li \image declarative-patharc.png
    \li \snippet qml/path/basicarc.qml 0
    \endtable

    Note that a single PathArc cannot be used to specify a circle. Instead, you can
    use two PathArc elements, each specifying half of the circle.

    \sa Path, PathLine, PathQuad, PathCubic, PathAngleArc, PathCurve, PathSvg
*/

/*!
    \qmlproperty real QtQuick::PathArc::x
    \qmlproperty real QtQuick::PathArc::y

    Defines the end point of the arc.

    \sa relativeX, relativeY
*/

/*!
    \qmlproperty real QtQuick::PathArc::relativeX
    \qmlproperty real QtQuick::PathArc::relativeY

    Defines the end point of the arc relative to its start.

    If both a relative and absolute end position are specified for a single axis, the relative
    position will be used.

    Relative and absolute positions can be mixed, for example it is valid to set a relative x
    and an absolute y.

    \sa x, y
*/

/*!
    \qmlproperty real QtQuick::PathArc::radiusX
    \qmlproperty real QtQuick::PathArc::radiusY

    Defines the radius of the arc.

    The following QML demonstrates how different radius values can be used to change
    the shape of the arc:
    \table
    \row
    \li \image declarative-arcradius.png
    \li \snippet qml/path/arcradius.qml 0
    \endtable
*/

qreal QQuickPathArc::radiusX() const
{
    return _radiusX;
}

void QQuickPathArc::setRadiusX(qreal radius)
{
    if (_radiusX == radius)
        return;

    _radiusX = radius;
    emit radiusXChanged();
    emit changed();
}

qreal QQuickPathArc::radiusY() const
{
    return _radiusY;
}

void QQuickPathArc::setRadiusY(qreal radius)
{
    if (_radiusY == radius)
        return;

    _radiusY = radius;
    emit radiusYChanged();
    emit changed();
}

/*!
    \qmlproperty bool QtQuick::PathArc::useLargeArc
    Whether to use a large arc as defined by the arc points.

    Given fixed start and end positions, radius, and direction,
    there are two possible arcs that can fit the data. useLargeArc
    is used to distinguish between these. For example, the following
    QML can produce either of the two illustrated arcs below by
    changing the value of useLargeArc.

    \table
    \row
    \li \image declarative-largearc.png
    \li \snippet qml/path/largearc.qml 0
    \endtable

    The default value is false.
*/

bool QQuickPathArc::useLargeArc() const
{
    return _useLargeArc;
}

void QQuickPathArc::setUseLargeArc(bool largeArc)
{
    if (_useLargeArc == largeArc)
        return;

    _useLargeArc = largeArc;
    emit useLargeArcChanged();
    emit changed();
}

/*!
    \qmlproperty enumeration QtQuick::PathArc::direction

    Defines the direction of the arc. Possible values are
    PathArc.Clockwise (default) and PathArc.Counterclockwise.

    The following QML can produce either of the two illustrated arcs below
    by changing the value of direction.
    \table
    \row
    \li \image declarative-arcdirection.png
    \li \snippet qml/path/arcdirection.qml 0
    \endtable

    \sa useLargeArc
*/

QQuickPathArc::ArcDirection QQuickPathArc::direction() const
{
    return _direction;
}

void QQuickPathArc::setDirection(ArcDirection direction)
{
    if (_direction == direction)
        return;

    _direction = direction;
    emit directionChanged();
    emit changed();
}

/*!
    \qmlproperty real QtQuick::PathArc::xAxisRotation

    Defines the rotation of the arc, in degrees. The default value is 0.

    An arc is a section of circles or ellipses. Given the radius and the start
    and end points, there are two ellipses that connect the points. This
    property defines the rotation of the X axis of these ellipses.

    \note The value is only useful when the x and y radius differ, meaning the
    arc is a section of ellipses.

    The following QML demonstrates how different radius values can be used to change
    the shape of the arc:
    \table
    \row
    \li \image declarative-arcrotation.png
    \li \snippet qml/path/arcrotation.qml 0
    \endtable
*/

qreal QQuickPathArc::xAxisRotation() const
{
    return _xAxisRotation;
}

void QQuickPathArc::setXAxisRotation(qreal rotation)
{
    if (_xAxisRotation == rotation)
        return;

    _xAxisRotation = rotation;
    emit xAxisRotationChanged();
    emit changed();
}

void QQuickPathArc::addToPath(QPainterPath &path, const QQuickPathData &data)
{
    const QPointF &startPoint = path.currentPosition();
    const QPointF &endPoint = positionForCurve(data, startPoint);
    QQuickSvgParser::pathArc(path,
            _radiusX,
            _radiusY,
            _xAxisRotation,
            _useLargeArc,
            _direction == Clockwise ? 1 : 0,
            endPoint.x(),
            endPoint.y(),
            startPoint.x(), startPoint.y());
}

/****************************************************************************/

/*!
    \qmltype PathAngleArc
    \instantiates QQuickPathAngleArc
    \inqmlmodule QtQuick
    \ingroup qtquick-animation-paths
    \brief Defines an arc with the given radii and center.

    PathAngleArc provides a simple way of specifying an arc. While PathArc is designed
    to work as part of a larger path (specifying start and end), PathAngleArc is designed
    to make a path where the arc is primary (such as a circular progress indicator) more intuitive.

    \sa Path, PathLine, PathQuad, PathCubic, PathCurve, PathSvg, PathArc
*/

/*!
    \qmlproperty real QtQuick::PathAngleArc::centerX
    \qmlproperty real QtQuick::PathAngleArc::centerY

    Defines the center of the arc.
*/

qreal QQuickPathAngleArc::centerX() const
{
    return _centerX;
}

void QQuickPathAngleArc::setCenterX(qreal centerX)
{
    if (_centerX == centerX)
        return;

    _centerX = centerX;
    emit centerXChanged();
    emit changed();
}

qreal QQuickPathAngleArc::centerY() const
{
    return _centerY;
}

void QQuickPathAngleArc::setCenterY(qreal centerY)
{
    if (_centerY == centerY)
        return;

    _centerY = centerY;
    emit centerYChanged();
    emit changed();
}

/*!
    \qmlproperty real QtQuick::PathAngleArc::radiusX
    \qmlproperty real QtQuick::PathAngleArc::radiusY

    Defines the radii of the ellipse of which the arc is part.
*/

qreal QQuickPathAngleArc::radiusX() const
{
    return _radiusX;
}

void QQuickPathAngleArc::setRadiusX(qreal radius)
{
    if (_radiusX == radius)
        return;

    _radiusX = radius;
    emit radiusXChanged();
    emit changed();
}

qreal QQuickPathAngleArc::radiusY() const
{
    return _radiusY;
}

void QQuickPathAngleArc::setRadiusY(qreal radius)
{
    if (_radiusY == radius)
        return;

    _radiusY = radius;
    emit radiusYChanged();
    emit changed();
}

/*!
    \qmlproperty real QtQuick::PathAngleArc::startAngle

    Defines the start angle of the arc.

    The start angle is reported clockwise, with zero degrees at the 3 o'clock position.
*/

qreal QQuickPathAngleArc::startAngle() const
{
    return _startAngle;
}

void QQuickPathAngleArc::setStartAngle(qreal angle)
{
    if (_startAngle == angle)
        return;

    _startAngle = angle;
    emit startAngleChanged();
    emit changed();
}

/*!
    \qmlproperty real QtQuick::PathAngleArc::sweepAngle

    Defines the sweep angle of the arc.

    The arc will begin at startAngle and continue sweepAngle degrees, with a value of 360
    resulting in a full circle. Positive numbers are clockwise and negative numbers are counterclockwise.
*/

qreal QQuickPathAngleArc::sweepAngle() const
{
    return _sweepAngle;
}

void QQuickPathAngleArc::setSweepAngle(qreal angle)
{
    if (_sweepAngle == angle)
        return;

    _sweepAngle = angle;
    emit sweepAngleChanged();
    emit changed();
}

/*!
    \qmlproperty bool QtQuick::PathAngleArc::moveToStart

    Whether this element should be disconnected from the previous Path element (or startX/Y).

    The default value is true. If set to false, the previous element's end-point
    (or startX/Y if PathAngleArc is the first element) will be connected to the arc's
    start-point with a straight line.
*/

bool QQuickPathAngleArc::moveToStart() const
{
    return _moveToStart;
}

void QQuickPathAngleArc::setMoveToStart(bool move)
{
    if (_moveToStart == move)
        return;

    _moveToStart = move;
    emit moveToStartChanged();
    emit changed();
}

void QQuickPathAngleArc::addToPath(QPainterPath &path, const QQuickPathData &)
{
    qreal x = _centerX - _radiusX;
    qreal y = _centerY - _radiusY;
    qreal width = _radiusX * 2;
    qreal height = _radiusY * 2;
    if (_moveToStart)
        path.arcMoveTo(x, y, width, height, -_startAngle);
    path.arcTo(x, y, width, height, -_startAngle, -_sweepAngle);
}

/****************************************************************************/

/*!
    \qmltype PathSvg
    \instantiates QQuickPathSvg
    \inqmlmodule QtQuick
    \ingroup qtquick-animation-paths
    \brief Defines a path using an SVG path data string.

    The following QML produces the path shown below:
    \table
    \row
    \li \image declarative-pathsvg.png
    \li
    \qml
    Path {
        startX: 50; startY: 50
        PathSvg { path: "L 150 50 L 100 150 z" }
    }
    \endqml
    \endtable

    \note Mixing PathSvg with other type of elements is not always supported.
    For example, when \l Shape is backed by \c{GL_NV_path_rendering}, a
    ShapePath can contain one or more PathSvg elements, or one or more other
    type of elements, but not both.

    \sa Path, PathLine, PathQuad, PathCubic, PathArc, PathAngleArc, PathCurve
*/

/*!
    \qmlproperty string QtQuick::PathSvg::path

    The SVG path data string specifying the path.

    See \l {http://www.w3.org/TR/SVG/paths.html#PathData}{W3C SVG Path Data}
    for more details on this format.
*/

QString QQuickPathSvg::path() const
{
    return _path;
}

void QQuickPathSvg::setPath(const QString &path)
{
    if (_path == path)
        return;

    _path = path;
    emit pathChanged();
    emit changed();
}

void QQuickPathSvg::addToPath(QPainterPath &path, const QQuickPathData &)
{
    QQuickSvgParser::parsePathDataFast(_path, path);
}

/****************************************************************************/

/*!
    \qmltype PathPercent
    \instantiates QQuickPathPercent
    \inqmlmodule QtQuick
    \ingroup qtquick-animation-paths
    \brief Manipulates the way a path is interpreted.

    PathPercent allows you to manipulate the spacing between items on a
    PathView's path. You can use it to bunch together items on part of
    the path, and spread them out on other parts of the path.

    The examples below show the normal distribution of items along a path
    compared to a distribution which places 50% of the items along the
    PathLine section of the path.
    \table
    \row
    \li \image declarative-nopercent.png
    \li
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
    \li \image declarative-percent.png
    \li
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
    \qmlproperty real QtQuick::PathPercent::value
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

qreal QQuickPathPercent::value() const
{
    return _value;
}

void QQuickPathPercent::setValue(qreal value)
{
    if (_value != value) {
        _value = value;
        emit valueChanged();
        emit changed();
    }
}

/*!
    \qmltype PathPolyline
    \instantiates QQuickPathPolyline
    \inqmlmodule QtQuick
    \ingroup qtquick-animation-paths
    \brief Defines a polyline through a list of coordinates.
    \since QtQuick 2.14

    The example below creates a triangular path consisting of four vertices
    on the edge of the containing Shape's bounding box.
    Through the containing shape's \l {QtQuick::Path::}{scale} property,
    the path will be rescaled together with its containing shape.

    \qml
    PathPolyline {
        id: ppl
        path: [ Qt.point(0.0, 0.0),
                Qt.point(1.0, 0.0),
                Qt.point(0.5, 1.0),
                Qt.point(0.0, 0.0)
              ]
    }
    \endqml

    \sa Path, PathQuad, PathCubic, PathArc, PathAngleArc, PathCurve, PathSvg, PathMove, PathPolyline
*/

/*!
    \qmlproperty point QtQuick::PathPolyline::start

    This read-only property contains the beginning of the polyline.
*/

/*!
    \qmlproperty list<point> QtQuick::PathPolyline::path

    This property defines the vertices of the polyline.

    It can be a JS array of points constructed with \c Qt.point(),
    a QList or QVector of QPointF, or QPolygonF.
    If you are binding this to a custom property in some C++ object,
    QPolygonF is the most appropriate type to use.
*/

QQuickPathPolyline::QQuickPathPolyline(QObject *parent) : QQuickCurve(parent)
{
}

QVariant QQuickPathPolyline::path() const
{
    return QVariant::fromValue(m_path);
}

void QQuickPathPolyline::setPath(const QVariant &path)
{
    if (path.userType() == QMetaType::QPolygonF) {
        setPath(path.value<QPolygonF>());
    } else if (path.canConvert<QVector<QPointF>>()) {
        setPath(path.value<QVector<QPointF>>());
    } else if (path.canConvert<QVariantList>()) {
        // This handles cases other than QPolygonF or QVector<QPointF>, such as
        // QList<QPointF>, QVector<QPoint>, QVariantList of QPointF, QVariantList of QPoint.
        QVector<QPointF> pathList;
        QVariantList vl = path.value<QVariantList>();
        // If path is a QJSValue, e.g. coming from a JS array of Qt.point() in QML,
        // then path.value<QVariantList>() is inefficient.
        // TODO We should be able to iterate over path.value<QSequentialIterable>() eventually
        for (const QVariant &v : vl)
            pathList.append(v.toPointF());
        setPath(pathList);
    } else {
        qWarning() << "PathPolyline: path of type" << path.userType() << "not supported";
    }
}

void QQuickPathPolyline::setPath(const QVector<QPointF> &path)
{
    if (m_path != path) {
        const QPointF &oldStart = start();
        m_path = path;
        const QPointF &newStart = start();
        emit pathChanged();
        if (oldStart != newStart)
            emit startChanged();
        emit changed();
    }
}

QPointF QQuickPathPolyline::start() const
{
    if (m_path.size()) {
        const QPointF &p = m_path.first();
        return p;
    }
    return QPointF();
}

void QQuickPathPolyline::addToPath(QPainterPath &path, const QQuickPathData &/*data*/)
{
    if (m_path.size() < 2)
        return;

    path.moveTo(m_path.first());
    for (int i = 1; i < m_path.size(); ++i)
        path.lineTo(m_path.at(i));
}


/*!
    \qmltype PathMultiline
    \instantiates QQuickPathMultiline
    \inqmlmodule QtQuick
    \ingroup qtquick-animation-paths
    \brief Defines a set of polylines through a list of lists of coordinates.
    \since QtQuick 2.14

    This element allows to define a list of polylines at once.
    Each polyline in the list will be preceded by a \l{QPainterPath::moveTo}{moveTo}
    command, effectively making each polyline a separate one.
    The polylines in this list are supposed to be non-intersecting with each other.
    In any case, when used in conjunction with a \l ShapePath, the containing ShapePath's
    \l ShapePath::fillRule applies.
    That is, with the default \c OddEvenFill and non intersecting shapes, the largest shape in the list defines an area to be filled;
    areas where two shapes overlap are holes; areas where three shapes overlap are filled areas inside holes, etc.

    The example below creates a high voltage symbol by adding each path
    of the symbol to the list of paths.
    The coordinates of the vertices are normalized, and through the containing shape's
    \l {QtQuick::Path::}{scale} property, the path will be rescaled together with its containing shape.

    \qml
    PathMultiline {
        paths: [
                [Qt.point(0.5,     0.06698),
                 Qt.point(1,       0.93301),
                 Qt.point(0,       0.93301),
                 Qt.point(0.5,     0.06698)],

                [Qt.point(0.5,     0.12472),
                 Qt.point(0.95,    0.90414),
                 Qt.point(0.05,    0.90414),
                 Qt.point(0.5,     0.12472)],

                [Qt.point(0.47131, 0.32986),
                 Qt.point(0.36229, 0.64789),
                 Qt.point(0.51492, 0.58590),
                 Qt.point(0.47563, 0.76014),
                 Qt.point(0.44950, 0.73590),
                 Qt.point(0.46292, 0.83392),
                 Qt.point(0.52162, 0.75190),
                 Qt.point(0.48531, 0.76230),
                 Qt.point(0.57529, 0.53189),
                 Qt.point(0.41261, 0.59189),
                 Qt.point(0.53001, 0.32786),
                 Qt.point(0.47131, 0.32986)]
               ]
    }
    \endqml

    \sa Path, QPainterPath::setFillRule, PathPolyline, PathQuad, PathCubic, PathArc, PathAngleArc, PathCurve, PathSvg, PathMove
*/

/*!
    \qmlproperty point QtQuick::PathMultiline::start

    This read-only property contains the beginning of the polylines.
*/

/*!
    \qmlproperty list<list<point>> QtQuick::PathMultiline::paths

    This property defines the vertices of the polylines.

    It can be a JS array of JS arrays of points constructed with \c Qt.point(),
    a QList or QVector of QPolygonF, or QVector<QVector<QPointF>>.
    If you are binding this to a custom property in some C++ object,
    QVector<QPolygonF> or QVector<QVector<QPointF>> is the most
    appropriate type to use.
*/

QQuickPathMultiline::QQuickPathMultiline(QObject *parent) : QQuickCurve(parent)
{
}

QVariant QQuickPathMultiline::paths() const
{
    return QVariant::fromValue(m_paths);
}

void QQuickPathMultiline::setPaths(const QVariant &paths)
{
    if (paths.canConvert<QVector<QPolygonF>>()) {
        const QVector<QPolygonF> pathPolygons = paths.value<QVector<QPolygonF>>();
        QVector<QVector<QPointF>> pathVectors;
        for (const QPolygonF &p : pathPolygons)
            pathVectors << p;
        setPaths(pathVectors);
    } else if (paths.canConvert<QVector<QVector<QPointF>>>()) {
        setPaths(paths.value<QVector<QVector<QPointF>>>());
    } else if (paths.canConvert<QVariantList>()) {
        // This handles cases other than QVector<QPolygonF> or QVector<QVector<QPointF>>, such as
        // QList<QVector<QPointF>>, QList<QList<QPointF>>, QVariantList of QVector<QPointF>,
        // QVariantList of QVariantList of QPointF, QVector<QList<QPoint>> etc.
        QVector<QVector<QPointF>> pathsList;
        QVariantList vll = paths.value<QVariantList>();
        for (const QVariant &v : vll) {
            // If we bind a QVector<QPolygonF> property directly, rather than via QVariant,
            // it will come through as QJSValue that can be converted to QVariantList of QPolygonF.
            if (v.canConvert<QPolygonF>()) {
                pathsList.append(v.value<QPolygonF>());
            } else {
                QVariantList vl = v.value<QVariantList>();
                QVector<QPointF> l;
                for (const QVariant &point : vl) {
                    if (point.canConvert<QPointF>())
                        l.append(point.toPointF());
                }
                if (l.size() >= 2)
                    pathsList.append(l);
            }
        }
        setPaths(pathsList);
    } else {
        qWarning() << "PathMultiline: paths of type" << paths.userType() << "not supported";
        setPaths(QVector<QVector<QPointF>>());
    }
}

void QQuickPathMultiline::setPaths(const QVector<QVector<QPointF>> &paths)
{
    if (m_paths != paths) {
        const QPointF &oldStart = start();
        m_paths = paths;
        const QPointF &newStart = start();
        emit pathsChanged();
        if (oldStart != newStart)
            emit startChanged();
        emit changed();
    }
}

QPointF QQuickPathMultiline::start() const
{
    if (m_paths.size())
        return m_paths.first().first();
    return QPointF();
}

void QQuickPathMultiline::addToPath(QPainterPath &path, const QQuickPathData &)
{
    if (!m_paths.size())
        return;
    for (const QVector<QPointF> &p: m_paths) {
        path.moveTo(p.first());
        for (int i = 1; i < p.size(); ++i)
            path.lineTo(p.at(i));
    }
}

/*!
    \qmltype PathText
    \instantiates QQuickPathText
    \inqmlmodule QtQuick
    \ingroup qtquick-animation-paths
    \brief Defines a string in a specified font.
    \since QtQuick 2.15

    This element defines the shape of a specified string in a specified font. The text's
    baseline will be translated to the x and y coordinates, and the outlines from the font
    will be added to the path accordingly.

    \qml
    PathText {
        x: 0
        y: font.pixelSize
        font.family: "Arial"
        font.pixelSize: 100
        text: "Foobar"
    }
    \endqml

    \sa Path, QPainterPath::setFillRule, PathPolyline, PathQuad, PathCubic, PathArc, PathAngleArc, PathCurve, PathSvg, PathMove
*/

/*!
    \qmlproperty real QtQuick::PathText::x

    The horizontal position of the PathText's baseline.
*/

/*!
    \qmlproperty real QtQuick::PathText::y

    The vertical position of the PathText's baseline.

    \note This property refers to the position of the baseline of the text, not the top of its bounding box. This may
    cause some confusion, e.g. when using the PathText with Qt Quick Shapes. See \l FontMetrics for information on how to
    get the ascent of a font, which can be used to translate the text into the expected position.
*/

/*!
    \qmlproperty string QtQuick::PathText::text

    The text for which this PathText should contain the outlines.
*/

/*!
    \qmlproperty string QtQuick::PathText::font.family

    Sets the family name of the font.

    The family name is case insensitive and may optionally include a foundry name, e.g. "Helvetica [Cronyx]".
    If the family is available from more than one foundry and the foundry isn't specified, an arbitrary foundry is chosen.
    If the family isn't available a family will be set using the font matching algorithm.
*/

/*!
    \qmlproperty string QtQuick::PathText::font.styleName

    Sets the style name of the font.

    The style name is case insensitive. If set, the font will be matched against style name instead
    of the font properties \l font.weight, \l font.bold and \l font.italic.
*/

/*!
    \qmlproperty bool QtQuick::PathText::font.bold

    Sets whether the font weight is bold.
*/

/*!
    \qmlproperty enumeration QtQuick::PathText::font.weight

    Sets the font's weight.

    The weight can be one of:
    \list
    \li Font.Thin
    \li Font.Light
    \li Font.ExtraLight
    \li Font.Normal - the default
    \li Font.Medium
    \li Font.DemiBold
    \li Font.Bold
    \li Font.ExtraBold
    \li Font.Black
    \endlist

    \qml
    PathText { text: "Hello"; font.weight: Font.DemiBold }
    \endqml
*/

/*!
    \qmlproperty bool QtQuick::PathText::font.italic

    Sets whether the font has an italic style.
*/

/*!
    \qmlproperty bool QtQuick::PathText::font.underline

    Sets whether the text is underlined.
*/

/*!
    \qmlproperty bool QtQuick::PathText::font.strikeout

    Sets whether the font has a strikeout style.
*/

/*!
    \qmlproperty real QtQuick::PathText::font.pointSize

    Sets the font size in points. The point size must be greater than zero.
*/

/*!
    \qmlproperty int QtQuick::PathText::font.pixelSize

    Sets the font size in pixels.

    Using this function makes the font device dependent.
    Use \c pointSize to set the size of the font in a device independent manner.
*/

/*!
    \qmlproperty real QtQuick::PathText::font.letterSpacing

    Sets the letter spacing for the font.

    Letter spacing changes the default spacing between individual letters in the font.
    A positive value increases the letter spacing by the corresponding pixels; a negative value decreases the spacing.
*/

/*!
    \qmlproperty real QtQuick::PathText::font.wordSpacing

    Sets the word spacing for the font.

    Word spacing changes the default spacing between individual words.
    A positive value increases the word spacing by a corresponding amount of pixels,
    while a negative value decreases the inter-word spacing accordingly.
*/

/*!
    \qmlproperty enumeration QtQuick::PathText::font.capitalization

    Sets the capitalization for the text.

    \list
    \li Font.MixedCase - This is the normal text rendering option where no capitalization change is applied.
    \li Font.AllUppercase - This alters the text to be rendered in all uppercase type.
    \li Font.AllLowercase - This alters the text to be rendered in all lowercase type.
    \li Font.SmallCaps - This alters the text to be rendered in small-caps type.
    \li Font.Capitalize - This alters the text to be rendered with the first character of each word as an uppercase character.
    \endlist

    \qml
    PathText { text: "Hello"; font.capitalization: Font.AllLowercase }
    \endqml
*/

/*!
    \qmlproperty bool QtQuick::PathText::font.kerning

    Enables or disables the kerning OpenType feature when shaping the text. Disabling this may
    improve performance when creating or changing the text, at the expense of some cosmetic
    features. The default value is true.

    \qml
    PathText { text: "OATS FLAVOUR WAY"; font.kerning: false }
    \endqml
*/

/*!
    \qmlproperty bool QtQuick::PathText::font.preferShaping

    Sometimes, a font will apply complex rules to a set of characters in order to
    display them correctly. In some writing systems, such as Brahmic scripts, this is
    required in order for the text to be legible, but in e.g. Latin script, it is merely
    a cosmetic feature. Setting the \c preferShaping property to false will disable all
    such features when they are not required, which will improve performance in most cases.

    The default value is true.

    \qml
    PathText { text: "Some text"; font.preferShaping: false }
    \endqml
*/

void QQuickPathText::updatePath() const
{
    if (!_path.isEmpty())
        return;

    _path.addText(0.0, 0.0, _font, _text);

    // Account for distance from baseline to top, since addText() takes baseline position
    QRectF brect = _path.boundingRect();
    _path.translate(_x, _y - brect.y());
}

void QQuickPathText::addToPath(QPainterPath &path)
{
    if (_text.isEmpty())
        return;
    updatePath();
    path.addPath(_path);
}

QT_END_NAMESPACE

#include "moc_qquickpath_p.cpp"
