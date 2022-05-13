// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickstyle.h"
#include "qquickstyle_p.h"
#include "qquickstyleoption.h"

#include <QtGui/qpainter.h>
#include <QtGui/qbitmap.h>
#include <QtGui/qpixmapcache.h>
#include <QtGui/qpa/qplatformtheme.h>

#include <QtGui/private/qguiapplication_p.h>

#ifndef QT_NO_DEBUG
#    include <QtCore/qdebug.h>
#endif

#include <limits.h>
#include <algorithm>

QT_BEGIN_NAMESPACE

namespace QQC2 {

/*!
    Constructs a style object.
*/
QStyle::QStyle()
    : QObject(*new QStylePrivate)
{
    Q_D(QStyle);
    d->proxyStyle = this;
}

/*!
    \internal

    Constructs a style object.
*/
QStyle::QStyle(QStylePrivate &dd)
    : QObject(dd)
{
    Q_D(QStyle);
    d->proxyStyle = this;
}

/*!
    Destroys the style object.
*/
QStyle::~QStyle()
{
}

/*!
    \fn QRect QStyle::itemTextRect(const QFontMetrics &metrics, const QRect &rectangle, int alignment, bool enabled, const QString &text) const

    Returns the area within the given \a rectangle in which to draw
    the provided \a text according to the specified font \a metrics
    and \a alignment. The \a enabled parameter indicates whether or
    not the associated item is enabled.

    If the given \a rectangle is larger than the area needed to render
    the \a text, the rectangle that is returned will be offset within
    \a rectangle according to the specified \a alignment.  For
    example, if \a alignment is Qt::AlignCenter, the returned
    rectangle will be centered within \a rectangle. If the given \a
    rectangle is smaller than the area needed, the returned rectangle
    will be the smallest rectangle large enough to render the \a text.

    \sa Qt::Alignment
*/
QRect QStyle::itemTextRect(const QFontMetrics &metrics, const QRect &rect, int alignment, bool enabled,
                       const QString &text) const
{
    QRect result;
    int x, y, w, h;
    rect.getRect(&x, &y, &w, &h);
    if (!text.isEmpty()) {
        result = metrics.boundingRect(x, y, w, h, alignment, text);
        if (!enabled && proxy()->styleHint(SH_EtchDisabledText)) {
            result.setWidth(result.width()+1);
            result.setHeight(result.height()+1);
        }
    } else {
        result = QRect(x, y, w, h);
    }
    return result;
}

/*!
    \fn QRect QStyle::itemPixmapRect(const QRect &rectangle, int alignment, const QPixmap &pixmap) const

    Returns the area within the given \a rectangle in which to draw
    the specified \a pixmap according to the defined \a alignment.
*/
QRect QStyle::itemPixmapRect(const QRect &rect, int alignment, const QPixmap &pixmap) const
{
    QRect result;
    int x, y, w, h;
    rect.getRect(&x, &y, &w, &h);

    const int pixmapWidth = pixmap.width()/pixmap.devicePixelRatio();
    const int pixmapHeight = pixmap.height()/pixmap.devicePixelRatio();

    if ((alignment & Qt::AlignVCenter) == Qt::AlignVCenter)
        y += h/2 - pixmapHeight/2;
    else if ((alignment & Qt::AlignBottom) == Qt::AlignBottom)
        y += h - pixmapHeight;
    if ((alignment & Qt::AlignRight) == Qt::AlignRight)
        x += w - pixmapWidth;
    else if ((alignment & Qt::AlignHCenter) == Qt::AlignHCenter)
        x += w/2 - pixmapWidth/2;
    else if ((alignment & Qt::AlignLeft) != Qt::AlignLeft && QGuiApplication::isRightToLeft())
        x += w - pixmapWidth;
    result = QRect(x, y, pixmapWidth, pixmapHeight);
    return result;
}

/*!
    \fn void QStyle::drawItemText(QPainter *painter, const QRect &rectangle, int alignment, const QPalette &palette, bool enabled, const QString& text, QPalette::ColorRole textRole) const

    Draws the given \a text in the specified \a rectangle using the
    provided \a painter and \a palette.

    The text is drawn using the painter's pen, and aligned and wrapped
    according to the specified \a alignment. If an explicit \a
    textRole is specified, the text is drawn using the \a palette's
    color for the given role. The \a enabled parameter indicates
    whether or not the item is enabled; when reimplementing this
    function, the \a enabled parameter should influence how the item is
    drawn.

    \sa Qt::Alignment, drawItemPixmap()
*/
void QStyle::drawItemText(QPainter *painter, const QRect &rect, int alignment, const QPalette &pal,
                          bool enabled, const QString& text, QPalette::ColorRole textRole) const
{
    if (text.isEmpty())
        return;
    QPen savedPen;
    if (textRole != QPalette::NoRole) {
        savedPen = painter->pen();
        painter->setPen(QPen(pal.brush(textRole), savedPen.widthF()));
    }
    if (!enabled) {
        if (proxy()->styleHint(SH_DitherDisabledText)) {
            QRect br;
            painter->drawText(rect, alignment, text, &br);
            painter->fillRect(br, QBrush(painter->background().color(), Qt::Dense5Pattern));
            return;
        } else if (proxy()->styleHint(SH_EtchDisabledText)) {
            QPen pen = painter->pen();
            painter->setPen(pal.light().color());
            painter->drawText(rect.adjusted(1, 1, 1, 1), alignment, text);
            painter->setPen(pen);
        }
    }
    painter->drawText(rect, alignment, text);
    if (textRole != QPalette::NoRole)
        painter->setPen(savedPen);
}

/*!
    \fn void QStyle::drawItemPixmap(QPainter *painter, const QRect &rectangle, int alignment,
                            const QPixmap &pixmap) const

    Draws the given \a pixmap in the specified \a rectangle, according
    to the specified \a alignment, using the provided \a painter.

    \sa drawItemText()
*/

void QStyle::drawItemPixmap(QPainter *painter, const QRect &rect, int alignment,
                            const QPixmap &pixmap) const
{
    qreal scale = pixmap.devicePixelRatio();
    QRect aligned = alignedRect(QGuiApplication::layoutDirection(), QFlag(alignment), pixmap.size() / scale, rect);
    QRect inter = aligned.intersected(rect);

    painter->drawPixmap(inter.x(), inter.y(), pixmap, inter.x() - aligned.x(), inter.y() - aligned.y(), inter.width() * scale, inter.height() *scale);
}

/*!
    \fn QRect QStyle::visualRect(Qt::LayoutDirection direction, const QRect &boundingRectangle, const QRect &logicalRectangle)

    Returns the given \a logicalRectangle converted to screen
    coordinates based on the specified \a direction. The \a
    boundingRectangle is used when performing the translation.

    This function is provided to support right-to-left desktops, and
    is typically used in implementations of the subControlRect()
    function.

    \sa QWidget::layoutDirection
*/
QRect QStyle::visualRect(Qt::LayoutDirection direction, const QRect &boundingRect, const QRect &logicalRect)
{
    if (direction == Qt::LeftToRight)
        return logicalRect;
    QRect rect = logicalRect;
    rect.translate(2 * (boundingRect.right() - logicalRect.right()) +
                   logicalRect.width() - boundingRect.width(), 0);
    return rect;
}

/*!
    \fn QPoint QStyle::visualPos(Qt::LayoutDirection direction, const QRect &boundingRectangle, const QPoint &logicalPosition)

    Returns the given \a logicalPosition converted to screen
    coordinates based on the specified \a direction.  The \a
    boundingRectangle is used when performing the translation.

    \sa QWidget::layoutDirection
*/
QPoint QStyle::visualPos(Qt::LayoutDirection direction, const QRect &boundingRect, const QPoint &logicalPos)
{
    if (direction == Qt::LeftToRight)
        return logicalPos;
    return QPoint(boundingRect.right() - logicalPos.x(), logicalPos.y());
}

/*!
     Returns a new rectangle of the specified \a size that is aligned to the given \a
     rectangle according to the specified \a alignment and \a direction.
 */
QRect QStyle::alignedRect(Qt::LayoutDirection direction, Qt::Alignment alignment, const QSize &size, const QRect &rectangle)
{
    alignment = visualAlignment(direction, alignment);
    int x = rectangle.x();
    int y = rectangle.y();
    int w = size.width();
    int h = size.height();
    if ((alignment & Qt::AlignVCenter) == Qt::AlignVCenter)
        y += rectangle.size().height()/2 - h/2;
    else if ((alignment & Qt::AlignBottom) == Qt::AlignBottom)
        y += rectangle.size().height() - h;
    if ((alignment & Qt::AlignRight) == Qt::AlignRight)
        x += rectangle.size().width() - w;
    else if ((alignment & Qt::AlignHCenter) == Qt::AlignHCenter)
        x += rectangle.size().width()/2 - w/2;
    return QRect(x, y, w, h);
}

/*!
  Transforms an \a alignment of Qt::AlignLeft or Qt::AlignRight
  without Qt::AlignAbsolute into Qt::AlignLeft or Qt::AlignRight with
  Qt::AlignAbsolute according to the layout \a direction. The other
  alignment flags are left untouched.

  If no horizontal alignment was specified, the function returns the
  default alignment for the given layout \a direction.

  QWidget::layoutDirection
*/
Qt::Alignment QStyle::visualAlignment(Qt::LayoutDirection direction, Qt::Alignment alignment)
{
    return QGuiApplicationPrivate::visualAlignment(direction, alignment);
}

/*!
    Converts the given \a logicalValue to a pixel position. The \a min
    parameter maps to 0, \a max maps to \a span and other values are
    distributed evenly in-between.

    This function can handle the entire integer range without
    overflow, providing that \a span is less than 4096.

    By default, this function assumes that the maximum value is on the
    right for horizontal items and on the bottom for vertical items.
    Set the \a upsideDown parameter to true to reverse this behavior.

    \sa sliderValueFromPosition()
*/

int QStyle::sliderPositionFromValue(int min, int max, int logicalValue, int span, bool upsideDown)
{
    if (span <= 0 || logicalValue < min || max <= min)
        return 0;
    if (logicalValue > max)
        return upsideDown ? span : min;

    uint range = max - min;
    uint p = upsideDown ? max - logicalValue : logicalValue - min;

    if (range > (uint)INT_MAX/4096) {
        double dpos = (double(p))/(double(range)/span);
        return int(dpos);
    } else if (range > (uint)span) {
        return (2 * p * span + range) / (2*range);
    } else {
        uint div = span / range;
        uint mod = span % range;
        return p * div + (2 * p * mod + range) / (2 * range);
    }
    // equiv. to (p * span) / range + 0.5
    // no overflow because of this implicit assumption:
    // span <= 4096
}

/*!
    \fn int QStyle::sliderValueFromPosition(int min, int max, int position, int span, bool upsideDown)

    Converts the given pixel \a position to a logical value. 0 maps to
    the \a min parameter, \a span maps to \a max and other values are
    distributed evenly in-between.

    This function can handle the entire integer range without
    overflow.

    By default, this function assumes that the maximum value is on the
    right for horizontal items and on the bottom for vertical
    items. Set the \a upsideDown parameter to true to reverse this
    behavior.

    \sa sliderPositionFromValue()
*/

int QStyle::sliderValueFromPosition(int min, int max, int pos, int span, bool upsideDown)
{
    if (span <= 0 || pos <= 0)
        return upsideDown ? max : min;
    if (pos >= span)
        return upsideDown ? min : max;

    uint range = max - min;

    if ((uint)span > range) {
        int tmp = (2 * pos * range + span) / (2 * span);
        return upsideDown ? max - tmp : tmp + min;
    } else {
        uint div = range / span;
        uint mod = range % span;
        int tmp = pos * div + (2 * pos * mod + span) / (2 * span);
        return upsideDown ? max - tmp : tmp + min;
    }
    // equiv. to min + (pos*range)/span + 0.5
    // no overflow because of this implicit assumption:
    // pos <= span < sqrt(INT_MAX+0.0625)+0.25 ~ sqrt(INT_MAX)
}

/*!
     Returns the style's standard palette.

    Note that on systems that support system colors, the style's
    standard palette is not used. In particular, the Windows
    Vista and Mac styles do not use the standard palette, but make
    use of native theme engines. With these styles, you should not set
    the palette with QApplication::setPalette().

    \sa QApplication::setPalette()
 */
QPalette QStyle::standardPalette() const
{
    QColor background = QColor(0xd4, 0xd0, 0xc8); // win 2000 grey

    QColor light(background.lighter());
    QColor dark(background.darker());
    QColor mid(Qt::gray);
    QPalette palette(Qt::black, background, light, dark, mid, Qt::black, Qt::white);
    palette.setBrush(QPalette::Disabled, QPalette::WindowText, dark);
    palette.setBrush(QPalette::Disabled, QPalette::Text, dark);
    palette.setBrush(QPalette::Disabled, QPalette::ButtonText, dark);
    palette.setBrush(QPalette::Disabled, QPalette::Base, background);
    return palette;
}

//Windows and KDE allow menus to cover the taskbar, while GNOME and macOS don't
bool QStylePrivate::useFullScreenForPopup()
{
    auto theme = QGuiApplicationPrivate::platformTheme();
    return theme && theme->themeHint(QPlatformTheme::UseFullScreenForPopupMenu).toBool();
}

} // namespace QQC2

QT_END_NAMESPACE

#include "moc_qquickstyle.cpp"
