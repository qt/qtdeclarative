// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickmaterialtextcontainer_p.h"

#include <QtCore/qpropertyanimation.h>
#include <QtGui/qpainter.h>
#include <QtGui/qpainterpath.h>
#include <QtQml/qqmlinfo.h>

QT_BEGIN_NAMESPACE

/*
    This class exists because:

    - Rectangle doesn't support individual radii for each corner (QTBUG-48774).
    - We need to draw an interrupted (where the placeholder text is) line for outlined containers.
    - We need to animate the focus line for filled containers, and we can't use "Behavior on"
      syntax because we only want to animate activeFocus becoming true, not also false. To do this
      requires imperative code, and we want to keep the QML declarative.

    focusAnimationProgress has to be a property even though it's only used internally,
    because we have to use QPropertyAnimation on it.

    An advantage of doing the animation in C++ is that we avoid the memory
    overhead of an animation instance even when we're not using it, and instead
    create it on demand and delete it when it's done. I tried doing the animation
    declaratively with states and transitions, but it was more difficult to implement
    and would have been harder to maintain, as well as having more overhead.
*/

QQuickMaterialTextContainer::QQuickMaterialTextContainer(QQuickItem *parent)
    : QQuickPaintedItem(parent)
{
}

bool QQuickMaterialTextContainer::isFilled() const
{
    return m_filled;
}

void QQuickMaterialTextContainer::setFilled(bool filled)
{
    if (filled == m_filled)
        return;

    m_filled = filled;
    update();
}

QColor QQuickMaterialTextContainer::fillColor() const
{
    return m_fillColor;
}

void QQuickMaterialTextContainer::setFillColor(const QColor &fillColor)
{
    if (fillColor == m_fillColor)
        return;

    m_fillColor = fillColor;
    update();
}

QColor QQuickMaterialTextContainer::outlineColor() const
{
    return m_outlineColor;
}

void QQuickMaterialTextContainer::setOutlineColor(const QColor &outlineColor)
{
    if (outlineColor == m_outlineColor)
        return;

    m_outlineColor = outlineColor;
    update();
}

QColor QQuickMaterialTextContainer::focusedOutlineColor() const
{
    return m_outlineColor;
}

void QQuickMaterialTextContainer::setFocusedOutlineColor(const QColor &focusedOutlineColor)
{
    if (focusedOutlineColor == m_focusedOutlineColor)
        return;

    m_focusedOutlineColor = focusedOutlineColor;
    update();
}

qreal QQuickMaterialTextContainer::focusAnimationProgress() const
{
    return m_focusAnimationProgress;
}

void QQuickMaterialTextContainer::setFocusAnimationProgress(qreal progress)
{
    if (qFuzzyCompare(progress, m_focusAnimationProgress))
        return;

    m_focusAnimationProgress = progress;
    update();
}

qreal QQuickMaterialTextContainer::placeholderTextWidth() const
{
    return m_placeholderTextWidth;
}

void QQuickMaterialTextContainer::setPlaceholderTextWidth(qreal placeholderTextWidth)
{
    if (qFuzzyCompare(placeholderTextWidth, m_placeholderTextWidth))
        return;

    m_placeholderTextWidth = placeholderTextWidth;
    update();
}

QQuickMaterialTextContainer::PlaceHolderHAlignment QQuickMaterialTextContainer::placeholderTextHAlign() const
{
    return m_placeholderTextHAlign;
}

void QQuickMaterialTextContainer::setPlaceholderTextHAlign(PlaceHolderHAlignment placeholderTextHAlign)
{
    if (m_placeholderTextHAlign == placeholderTextHAlign)
        return;

    m_placeholderTextHAlign = placeholderTextHAlign;
    update();
}

bool QQuickMaterialTextContainer::controlHasActiveFocus() const
{
    return m_controlHasActiveFocus;
}

void QQuickMaterialTextContainer::setControlHasActiveFocus(bool controlHasActiveFocus)
{
    if (m_controlHasActiveFocus == controlHasActiveFocus)
        return;

    m_controlHasActiveFocus = controlHasActiveFocus;
    if (m_controlHasActiveFocus)
        controlGotActiveFocus();
    else
        controlLostActiveFocus();
    emit controlHasActiveFocusChanged();
}

bool QQuickMaterialTextContainer::controlHasText() const
{
    return m_controlHasText;
}

void QQuickMaterialTextContainer::setControlHasText(bool controlHasText)
{
    if (m_controlHasText == controlHasText)
        return;

    m_controlHasText = controlHasText;
    // TextArea's text length is updated after component completion,
    // so account for that here and in setPlaceholderHasText().
    maybeSetFocusAnimationProgress();
    update();
    emit controlHasTextChanged();
}

bool QQuickMaterialTextContainer::placeholderHasText() const
{
    return m_placeholderHasText;
}

void QQuickMaterialTextContainer::setPlaceholderHasText(bool placeholderHasText)
{
    if (m_placeholderHasText == placeholderHasText)
        return;

    m_placeholderHasText = placeholderHasText;
    maybeSetFocusAnimationProgress();
    update();
    emit placeholderHasTextChanged();
}

int QQuickMaterialTextContainer::horizontalPadding() const
{
    return m_horizontalPadding;
}

/*!
    \internal

    The text field's horizontal padding.

    We need this to be a property so that the QML can set it, since we can't
    access QQuickMaterialStyle's C++ API from this plugin.
*/
void QQuickMaterialTextContainer::setHorizontalPadding(int horizontalPadding)
{
    if (m_horizontalPadding == horizontalPadding)
        return;
    m_horizontalPadding = horizontalPadding;
    update();
    emit horizontalPaddingChanged();
}

void QQuickMaterialTextContainer::paint(QPainter *painter)
{
    qreal w = width();
    qreal h = height();
    if (w <= 0 || h <= 0)
        return;

    // Account for pen width.
    const qreal penWidth = m_filled ? 1 : (m_controlHasActiveFocus ? 2 : 1);
    w -= penWidth;
    h -= penWidth;

    const qreal cornerRadius = 4;
    // This is coincidentally the same as cornerRadius, but use different variable names
    // to keep the code understandable.
    const qreal gapPadding = 4;
    // When animating focus on outlined containers, we need to make a gap
    // at the top left for the placeholder text.
    // If the text is too wide for the container, it will be elided, so
    // we shouldn't need to clamp its width here. TODO: check that this is the case for TextArea.
    const qreal halfPlaceholderWidth = m_placeholderTextWidth / 2;
    // Take care of different Alignment cases for the placeholder text.
    qreal gapCenterX;
    switch (m_placeholderTextHAlign) {
    case PlaceHolderHAlignment::AlignHCenter:
        gapCenterX = width() / 2;
        break;
    case PlaceHolderHAlignment::AlignRight:
        gapCenterX = width()  - halfPlaceholderWidth - m_horizontalPadding;
        break;
    default:
        gapCenterX = m_horizontalPadding + halfPlaceholderWidth;
        break;
    }

    QPainterPath path;

    QPointF startPos;

    // Top-left rounded corner.
    if (m_filled || m_focusAnimationProgress == 0) {
        startPos = QPointF(cornerRadius, 0);
    } else {
        // Start at the center of the gap and animate outwards towards the left-hand side.
        // Subtract gapPadding to account for the gap between the line and the placeholder text.
        // Also subtract the pen width because otherwise it extends by that distance too much to the right.
        // Changing the cap style to Qt::FlatCap would only fix this by half the pen width,
        // but it has no effect anyway (perhaps it literally only affects end points and not "start" points?).
        startPos = QPointF(gapCenterX - (m_focusAnimationProgress * halfPlaceholderWidth) - gapPadding - penWidth, 0);
    }
    path.moveTo(startPos);
    path.arcTo(0, 0, cornerRadius * 2, cornerRadius * 2, 90, 90);

    // Bottom-left corner.
    if (m_filled) {
        path.lineTo(0, h);
    } else {
        path.lineTo(0, h - cornerRadius * 2);
        path.arcTo(0, h - cornerRadius * 2, cornerRadius * 2, cornerRadius * 2, 180, 90);
    }

    // Bottom-right corner.
    if (m_filled) {
        path.lineTo(w, h);
    } else {
        path.lineTo(w - cornerRadius * 2, h);
        path.arcTo(w - cornerRadius * 2, h - cornerRadius * 2, cornerRadius * 2, cornerRadius * 2, 270, 90);
    }

    // Top-right rounded corner.
    path.lineTo(w, cornerRadius);
    path.arcTo(w - (cornerRadius * 2), 0, cornerRadius * 2, cornerRadius * 2, 0, 90);

    if (m_filled || qFuzzyIsNull(m_focusAnimationProgress)) {
        // Back to the start.
        path.lineTo(startPos.x(), startPos.y());
    } else {
        path.lineTo(gapCenterX + (m_focusAnimationProgress * halfPlaceholderWidth) + gapPadding, startPos.y());
    }

    // Account for pen width.
    painter->translate(penWidth / 2, penWidth / 2);

    painter->setRenderHint(QPainter::Antialiasing, true);

    auto control = textControl();
    const bool focused = control && control->hasActiveFocus();
    // We still want to draw the stroke when it's filled, otherwise it will be a pixel
    // (the pen width) too narrow on either side.
    QPen pen;
    pen.setColor(m_filled ? m_fillColor : (focused ? m_focusedOutlineColor : m_outlineColor));
    pen.setWidthF(penWidth);
    painter->setPen(pen);
    if (m_filled)
        painter->setBrush(m_fillColor);

    // Fill or stroke the container's shape.
    // If not filling, the default brush will be used, which is Qt::NoBrush.
    painter->drawPath(path);

    // Draw the focus line at the bottom for filled containers.
    if (m_filled) {
        if (!qFuzzyCompare(m_focusAnimationProgress, qreal(1.0))) {
            // Draw the enabled active indicator line (#10) that's at the bottom when it's not focused:
            // https://m3.material.io/components/text-fields/specs#6d654d1d-262e-4697-858c-9a75e8e7c81d
            // Don't bother drawing it when the animation has finished, as the focused active indicator
            // line below will obscure it.
            pen.setColor(m_outlineColor);
            painter->setPen(pen);
            painter->drawLine(0, h, w, h);
        }

        if (!qFuzzyIsNull(m_focusAnimationProgress)) {
            // Draw the focused active indicator line (#6) that's at the bottom when it's focused.
            // Start at the center and expand outwards.
            const int lineLength = m_focusAnimationProgress * w;
            const int horizontalCenter = w / 2;
            pen.setColor(m_focusedOutlineColor);
            pen.setWidth(2);
            painter->setPen(pen);
            painter->drawLine(horizontalCenter - (lineLength / 2), h,
                horizontalCenter + (lineLength / 2) + pen.width() / 2, h);
        }
    }
}

bool QQuickMaterialTextContainer::shouldAnimateOutline() const
{
    return !m_controlHasText && m_placeholderHasText;
}

/*!
    \internal

    \sa QQuickPlaceholderText::textControl().
*/
QQuickItem *QQuickMaterialTextContainer::textControl() const
{
    return qobject_cast<QQuickItem *>(parent());
}

void QQuickMaterialTextContainer::controlGotActiveFocus()
{
    const bool shouldAnimate = m_filled ? !m_controlHasText : shouldAnimateOutline();
    if (!shouldAnimate) {
        // It does have focus, but sometimes we don't need to animate anything, just change colors.
        if (m_filled && m_controlHasText) {
            // When a filled container has text already entered, we should just immediately change
            // the color and thickness of the indicator line.
            m_focusAnimationProgress = 1;
        }
        update();
        return;
    }

    startFocusAnimation();
}

void QQuickMaterialTextContainer::controlLostActiveFocus()
{
    // We don't want to animate the active indicator line (at the bottom) of filled containers
    // when the control loses focus, only when it gets it.
    if (m_filled || !shouldAnimateOutline()) {
        // Ensure that we set this so that filled containers go back to a non-accent-colored
        // active indicator line when losing focus.
        if (m_filled)
            m_focusAnimationProgress = 0;
        update();
        return;
    }

    QPropertyAnimation *animation = new QPropertyAnimation(this, "focusAnimationProgress", this);
    animation->setDuration(300);
    animation->setStartValue(1);
    animation->setEndValue(0);
    animation->start(QAbstractAnimation::DeleteWhenStopped);
}

void QQuickMaterialTextContainer::startFocusAnimation()
{
    // Each time setFocusAnimationProgress is called by the animation, it'll call update(),
    // which will cause us to be re-rendered.
    QPropertyAnimation *animation = new QPropertyAnimation(this, "focusAnimationProgress", this);
    animation->setDuration(300);
    animation->setStartValue(0);
    animation->setEndValue(1);
    animation->start(QAbstractAnimation::DeleteWhenStopped);
}

void QQuickMaterialTextContainer::maybeSetFocusAnimationProgress()
{
    if (m_filled)
        return;

    if (m_controlHasText && m_placeholderHasText) {
        // Show the interrupted outline when there is text.
        setFocusAnimationProgress(1);
    } else if (!m_controlHasText && !m_controlHasActiveFocus) {
        // If the text was cleared while it didn't have focus, don't animate, just close the gap.
        setFocusAnimationProgress(0);
    }
}

void QQuickMaterialTextContainer::componentComplete()
{
    QQuickPaintedItem::componentComplete();

    if (!parentItem())
        qmlWarning(this) << "Expected parent item by component completion!";

    maybeSetFocusAnimationProgress();
}

QT_END_NAMESPACE
