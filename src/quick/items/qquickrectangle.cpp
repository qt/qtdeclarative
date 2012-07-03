/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the QtQml module of the Qt Toolkit.
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

#include "qquickrectangle_p.h"
#include "qquickrectangle_p_p.h"

#include <QtQuick/private/qsgcontext_p.h>
#include <private/qsgadaptationlayer_p.h>

#include <QtGui/qpixmapcache.h>
#include <QtCore/qstringbuilder.h>
#include <QtCore/qmath.h>
#include <QtCore/qmetaobject.h>

QT_BEGIN_NAMESPACE

// XXX todo - should we change rectangle to draw entirely within its width/height?
/*!
    \internal
    \class QQuickPen
    \brief For specifying a pen used for drawing rectangle borders on a QQuickView

    By default, the pen is invalid and nothing is drawn. You must either set a color (then the default
    width is 1) or a width (then the default color is black).

    A width of 1 indicates is a single-pixel line on the border of the item being painted.

    Example:
    \qml
    Rectangle {
        border.width: 2
        border.color: "red"
    }
    \endqml
*/

QQuickPen::QQuickPen(QObject *parent)
    : QObject(parent)
    , m_width(1)
    , m_color("#000000")
    , m_aligned(true)
    , m_valid(false)
{
}

qreal QQuickPen::width() const
{
    return m_width;
}

void QQuickPen::setWidth(qreal w)
{
    if (m_width == w && m_valid)
        return;

    m_width = w;
    m_valid = m_color.alpha() && (qRound(m_width) >= 1 || (!m_aligned && m_width > 0));
    emit penChanged();
}

QColor QQuickPen::color() const
{
    return m_color;
}

void QQuickPen::setColor(const QColor &c)
{
    m_color = c;
    m_valid = m_color.alpha() && (qRound(m_width) >= 1 || (!m_aligned && m_width > 0));
    emit penChanged();
}

bool QQuickPen::aligned() const
{
    return m_aligned;
}

void QQuickPen::setAligned(bool aligned)
{
    if (aligned == m_aligned)
        return;
    m_aligned = aligned;
    m_valid = m_color.alpha() && (qRound(m_width) >= 1 || (!m_aligned && m_width > 0));
    emit penChanged();
}

bool QQuickPen::isValid() const
{
    return m_valid;
}

/*!
    \qmlclass GradientStop QQuickGradientStop
    \inqmlmodule QtQuick 2
    \ingroup qtquick-visual-utility
    \brief Defines the color at a position in a Gradient

    \sa Gradient
*/

/*!
    \qmlproperty real QtQuick2::GradientStop::position
    \qmlproperty color QtQuick2::GradientStop::color

    The position and color properties describe the color used at a given
    position in a gradient, as represented by a gradient stop.

    The default position is 0.0; the default color is black.

    \sa Gradient
*/
QQuickGradientStop::QQuickGradientStop(QObject *parent)
    : QObject(parent)
{
}

qreal QQuickGradientStop::position() const
{
    return m_position;
}

void QQuickGradientStop::setPosition(qreal position)
{
    m_position = position; updateGradient();
}

QColor QQuickGradientStop::color() const
{
    return m_color;
}

void QQuickGradientStop::setColor(const QColor &color)
{
    m_color = color; updateGradient();
}

void QQuickGradientStop::updateGradient()
{
    if (QQuickGradient *grad = qobject_cast<QQuickGradient*>(parent()))
        grad->doUpdate();
}

/*!
    \qmlclass Gradient QQuickGradient
    \inqmlmodule QtQuick 2
    \ingroup qtquick-visual-utility
    \brief Defines a gradient fill

    A gradient is defined by two or more colors, which will be blended seamlessly.

    The colors are specified as a set of GradientStop child items, each of
    which defines a position on the gradient from 0.0 to 1.0 and a color.
    The position of each GradientStop is defined by setting its
    \l{GradientStop::}{position} property; its color is defined using its
    \l{GradientStop::}{color} property.

    A gradient without any gradient stops is rendered as a solid white fill.

    Note that this item is not a visual representation of a gradient. To display a
    gradient, use a visual element (like \l Rectangle) which supports the use
    of gradients.

    \section1 Example Usage

    \div {class="float-right"}
    \inlineimage qml-gradient.png
    \enddiv

    The following example declares a \l Rectangle item with a gradient starting
    with red, blending to yellow at one third of the height of the rectangle,
    and ending with green:

    \snippet qml/gradient.qml code

    \clearfloat
    \section1 Performance and Limitations

    Calculating gradients can be computationally expensive compared to the use
    of solid color fills or images. Consider using gradients for static items
    in a user interface.

    In Qt 5.0, only vertical, linear gradients can be applied to items. If you
    need to apply different orientations of gradients, a combination of rotation
    and clipping will need to be applied to the relevant items. This can
    introduce additional performance requirements for your application.

    The use of animations involving gradient stops may not give the desired
    result. An alternative way to animate gradients is to use pre-generated
    images or SVG drawings containing gradients.

    \sa GradientStop
*/

/*!
    \qmlproperty list<GradientStop> QtQuick2::Gradient::stops
    \default

    This property holds the gradient stops describing the gradient.

    By default, this property contains an empty list.

    To set the gradient stops, define them as children of the Gradient element.
*/
QQuickGradient::QQuickGradient(QObject *parent)
: QObject(parent)
{
}

QQuickGradient::~QQuickGradient()
{
}

QQmlListProperty<QQuickGradientStop> QQuickGradient::stops()
{
    return QQmlListProperty<QQuickGradientStop>(this, m_stops);
}

void QQuickGradient::doUpdate()
{
    emit updated();
}

int QQuickRectanglePrivate::doUpdateSlotIdx = -1;

/*!
    \qmlclass Rectangle QQuickRectangle
    \inqmlmodule QtQuick 2
    \inherits Item
    \ingroup qtquick-visual
    \brief Describes a filled rectangle with an optional border

    Rectangle items are used to fill areas with solid color or gradients, and are
    often used to hold other items.

    \section1 Appearance

    Each Rectangle item is painted using either a solid fill color, specified using
    the \l color property, or a gradient, defined using a Gradient element and set
    using the \l gradient property. If both a color and a gradient are specified,
    the gradient is used.

    You can add an optional border to a rectangle with its own color and thickness
    by setting the \l border.color and \l border.width properties.

    You can also create rounded rectangles using the \l radius property. Since this
    introduces curved edges to the corners of a rectangle, it may be appropriate to
    set the \l smooth property to improve its appearance.

    \section1 Example Usage

    \div {class="float-right"}
    \inlineimage declarative-rect.png
    \enddiv

    The following example shows the effects of some of the common properties on a
    Rectangle item, which in this case is used to create a square:

    \snippet qml/rectangle/rectangle.qml document

    \clearfloat
    \section1 Performance

    Using the \l smooth property improves the appearance of a rounded rectangle at
    the cost of rendering performance. You should consider unsetting this property
    for rectangles in motion, and only set it when they are stationary.

    \sa Image
*/

QQuickRectangle::QQuickRectangle(QQuickItem *parent)
: QQuickItem(*(new QQuickRectanglePrivate), parent)
{
    setFlag(ItemHasContents);
}

void QQuickRectangle::doUpdate()
{
    Q_D(QQuickRectangle);
    qreal penMargin = 0;
    qreal penOffset = 0;
    if (d->pen && d->pen->isValid()) {
        if (d->pen->aligned()) {
            const int pw = qRound(d->pen->width());
            penMargin = qreal(0.5) * pw;
            penOffset = (pw & 1) * qreal(0.5);
        } else {
            penMargin = qreal(0.5) * d->pen->width();
        }
    }
    if (penMargin != d->penMargin || penOffset != d->penOffset) {
        d->penMargin = penMargin;
        d->penOffset = penOffset;
        d->dirty(QQuickItemPrivate::Size); // update clip
    }
    update();
}

/*!
    \qmlproperty int QtQuick2::Rectangle::border.width
    \qmlproperty color QtQuick2::Rectangle::border.color

    The width and color used to draw the border of the rectangle.

    A width of 1 creates a thin line. For no line, use a width of 0 or a transparent color.

    \note The width of the rectangle's border does not affect the geometry of the
    rectangle itself or its position relative to other items if anchors are used.

    If \c border.width is an odd number, the rectangle is painted at a half-pixel offset to retain
    border smoothness. Also, the border is rendered evenly on either side of the
    rectangle's boundaries, and the spare pixel is rendered to the right and below the
    rectangle (as documented for QRect rendering). This can cause unintended effects if
    \c border.width is 1 and the rectangle is \l{Item::clip}{clipped} by a parent item:

    \div {class="float-right"}
    \inlineimage rect-border-width.png
    \enddiv

    \snippet qml/rectangle/rect-border-width.qml 0

    \clearfloat
    Here, the innermost rectangle's border is clipped on the bottom and right edges by its
    parent. To avoid this, the border width can be set to two instead of one.
*/
QQuickPen *QQuickRectangle::border()
{
    Q_D(QQuickRectangle);
    return d->getPen();
}

/*!
    \qmlproperty Gradient QtQuick2::Rectangle::gradient

    The gradient to use to fill the rectangle.

    This property allows for the construction of simple vertical gradients.
    Other gradients may by formed by adding rotation to the rectangle.

    \div {class="float-left"}
    \inlineimage declarative-rect_gradient.png
    \enddiv

    \snippet qml/rectangle/rectangle-gradient.qml rectangles
    \clearfloat

    If both a gradient and a color are specified, the gradient will be used.

    \sa Gradient, color
*/
QQuickGradient *QQuickRectangle::gradient() const
{
    Q_D(const QQuickRectangle);
    return d->gradient;
}

void QQuickRectangle::setGradient(QQuickGradient *gradient)
{
    Q_D(QQuickRectangle);
    if (d->gradient == gradient)
        return;
    static int updatedSignalIdx = -1;
    if (updatedSignalIdx < 0)
        updatedSignalIdx = QMetaMethod::fromSignal(&QQuickGradient::updated).methodIndex();
    if (d->doUpdateSlotIdx < 0)
        d->doUpdateSlotIdx = QQuickRectangle::staticMetaObject.indexOfSlot("doUpdate()");
    if (d->gradient)
        QMetaObject::disconnect(d->gradient, updatedSignalIdx, this, d->doUpdateSlotIdx);
    d->gradient = gradient;
    if (d->gradient)
        QMetaObject::connect(d->gradient, updatedSignalIdx, this, d->doUpdateSlotIdx);
    update();
}

void QQuickRectangle::resetGradient()
{
    setGradient(0);
}

/*!
    \qmlproperty real QtQuick2::Rectangle::radius
    This property holds the corner radius used to draw a rounded rectangle.

    If radius is non-zero, the rectangle will be painted as a rounded rectangle, otherwise it will be
    painted as a normal rectangle. The same radius is used by all 4 corners; there is currently
    no way to specify different radii for different corners.
*/
qreal QQuickRectangle::radius() const
{
    Q_D(const QQuickRectangle);
    return d->radius;
}

void QQuickRectangle::setRadius(qreal radius)
{
    Q_D(QQuickRectangle);
    if (d->radius == radius)
        return;

    d->radius = radius;
    update();
    emit radiusChanged();
}

/*!
    \qmlproperty color QtQuick2::Rectangle::color
    This property holds the color used to fill the rectangle.

    The default color is white.

    \div {class="float-right"}
    \inlineimage rect-color.png
    \enddiv

    The following example shows rectangles with colors specified
    using hexadecimal and named color notation:

    \snippet qml/rectangle/rectangle-colors.qml rectangles

    \clearfloat
    If both a gradient and a color are specified, the gradient will be used.

    \sa gradient
*/
QColor QQuickRectangle::color() const
{
    Q_D(const QQuickRectangle);
    return d->color;
}

void QQuickRectangle::setColor(const QColor &c)
{
    Q_D(QQuickRectangle);
    if (d->color == c)
        return;

    d->color = c;
    update();
    emit colorChanged();
}

QSGNode *QQuickRectangle::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *data)
{
    Q_UNUSED(data);
    Q_D(QQuickRectangle);

    if (width() <= 0 || height() <= 0
            || (d->color.alpha() == 0 && (!d->pen || d->pen->width() == 0 || d->pen->color().alpha() == 0))) {
        delete oldNode;
        return 0;
    }

    QSGRectangleNode *rectangle = static_cast<QSGRectangleNode *>(oldNode);
    if (!rectangle) rectangle = d->sceneGraphContext()->createRectangleNode();

    rectangle->setRect(QRectF(0, 0, width(), height()));
    rectangle->setColor(d->color);

    if (d->pen && d->pen->isValid()) {
        rectangle->setPenColor(d->pen->color());
        rectangle->setPenWidth(d->pen->width());
        rectangle->setAligned(d->pen->aligned());
    } else {
        rectangle->setPenWidth(0);
    }

    rectangle->setRadius(d->radius);

    QGradientStops stops;
    if (d->gradient) {
        QList<QQuickGradientStop *> qxstops = d->gradient->m_stops;
        for (int i = 0; i < qxstops.size(); ++i){
            int j = 0;
            while (j < stops.size() && stops.at(j).first < qxstops[i]->position())
                j++;
            stops.insert(j, QGradientStop(qxstops.at(i)->position(), qxstops.at(i)->color()));
        }
    }
    rectangle->setGradientStops(stops);

    rectangle->update();

    return rectangle;
}
/*!
    \qmlproperty bool QtQuick2::Rectangle::smooth

    Set this property if you want the item to be smoothly scaled or
    transformed.  Smooth filtering gives better visual quality, but is slower.  If
    the item is displayed at its natural size, this property has no visual or
    performance effect.

    \note Generally scaling artifacts are only visible if the item is stationary on
    the screen.  A common pattern when animating an item is to disable smooth
    filtering at the beginning of the animation and reenable it at the conclusion.

    \image rect-smooth.png
    On this image, smooth is turned off for the top half and on for the bottom half.
*/

QRectF QQuickRectangle::boundingRect() const
{
    Q_D(const QQuickRectangle);
    return QRectF(d->penOffset - d->penMargin, d->penOffset - d->penMargin,
                  d->width + 2 * d->penMargin, d->height + 2 * d->penMargin);
}

QRectF QQuickRectangle::clipRect() const
{
    return QQuickRectangle::boundingRect();
}

QT_END_NAMESPACE
