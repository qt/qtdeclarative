// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickmaterialplaceholdertext_p.h"

#include <QtCore/qpropertyanimation.h>
#include <QtCore/qparallelanimationgroup.h>
#include <QtGui/qpainter.h>
#include <QtGui/qpainterpath.h>
#include <QtQml/qqmlinfo.h>
#include <QtQuickTemplates2/private/qquicktheme_p.h>
#include <QtQuickTemplates2/private/qquicktextarea_p.h>
#include <QtQuickTemplates2/private/qquicktextfield_p.h>

QT_BEGIN_NAMESPACE

static const qreal floatingScale = 0.8;
Q_GLOBAL_STATIC(QEasingCurve, animationEasingCurve, QEasingCurve::OutSine);

/*
    This class makes it easier to animate the various placeholder text changes
    for each type of text container (filled, outlined).

    By doing animations in C++, we avoid having a bunch of states, transitions,
    and animations (which are all QObjects) declared in QML, even if that text
    control never gets focus and hence never needs them.
*/

QQuickMaterialPlaceholderText::QQuickMaterialPlaceholderText(QQuickItem *parent)
    : QQuickPlaceholderText(parent)
{
    connect(this, &QQuickMaterialPlaceholderText::effectiveHorizontalAlignmentChanged,
            this, &QQuickMaterialPlaceholderText::adjustTransformOrigin);
}

bool QQuickMaterialPlaceholderText::isFilled() const
{
    return m_filled;
}

void QQuickMaterialPlaceholderText::setFilled(bool filled)
{
    if (filled == m_filled)
        return;

    m_filled = filled;
    update();
    void filledChanged();
}

bool QQuickMaterialPlaceholderText::controlHasActiveFocus() const
{
    return m_controlHasActiveFocus;
}

void QQuickMaterialPlaceholderText::setControlHasActiveFocus(bool controlHasActiveFocus)
{
    if (m_controlHasActiveFocus == controlHasActiveFocus)
        return;

    m_controlHasActiveFocus = controlHasActiveFocus;
    controlActiveFocusChanged();
    emit controlHasActiveFocusChanged();
}

bool QQuickMaterialPlaceholderText::controlHasText() const
{
    return m_controlHasText;
}

void QQuickMaterialPlaceholderText::setControlHasText(bool controlHasText)
{
    if (m_controlHasText == controlHasText)
        return;

    m_controlHasText = controlHasText;
    updateFocusAnimation();
    emit controlHasTextChanged();
}

/*
    Placeholder text of outlined text fields should float when:
    - There is placeholder text, and
      - The control has active focus, or
      - The control has text
*/
bool QQuickMaterialPlaceholderText::shouldFloat() const
{
    const bool controlHasActiveFocusOrText = m_controlHasActiveFocus || m_controlHasText;
    return m_filled
        ? controlHasActiveFocusOrText
        : !text().isEmpty() && controlHasActiveFocusOrText;
}

bool QQuickMaterialPlaceholderText::shouldAnimate() const
{
    return m_filled
        ? !m_controlHasText
        : !m_controlHasText && !text().isEmpty();
}

void QQuickMaterialPlaceholderText::updateY()
{
    setY(shouldFloat() ? floatingTargetY() : normalTargetY());
}

void QQuickMaterialPlaceholderText::updateX()
{
    setX(shouldFloat() ? floatingTargetX() : normalTargetX());
}

qreal controlTopInset(QQuickItem *textControl)
{
    if (const auto textArea = qobject_cast<QQuickTextArea *>(textControl))
        return textArea->topInset();

    if (const auto textField = qobject_cast<QQuickTextField *>(textControl))
        return textField->topInset();

    return 0;
}

qreal QQuickMaterialPlaceholderText::normalTargetY() const
{
    auto *textArea = qobject_cast<QQuickTextArea *>(textControl());
    if (textArea && m_controlHeight >= textArea->implicitHeight()) {
        // TextArea can be multiple lines in height, and we want the
        // placeholder text to sit in the middle of its default-height
        // (one-line) if its explicit height is greater than or equal to its
        // implicit height - i.e. if it has room for it. If it doesn't have
        // room, just do what TextField does.
        // We should also account for any topInset the user might have specified,
        // which is useful to ensure that the text doesn't get clipped.
        return ((m_controlImplicitBackgroundHeight - m_largestHeight) / 2.0)
            + controlTopInset(textControl());
    }

    // When the placeholder text shouldn't float, it should sit in the middle of the TextField.
    return (m_controlHeight - height()) / 2.0;
}

qreal QQuickMaterialPlaceholderText::floatingTargetY() const
{
    // For filled text fields, the placeholder text sits just above
    // the text when floating.
    if (m_filled)
        return m_verticalPadding;

    // Outlined text fields have the placeaholder vertically centered
    // along the outline at the top.
    return (-m_largestHeight / 2.0) + controlTopInset(textControl());
}

qreal QQuickMaterialPlaceholderText::normalTargetX() const
{
    return m_leftPadding;
}

qreal QQuickMaterialPlaceholderText::floatingTargetX() const
{
    return m_floatingLeftPadding;
}

/*!
    \internal

    The height of the text at its largest size that we set.
*/
int QQuickMaterialPlaceholderText::largestHeight() const
{
    return m_largestHeight;
}

qreal QQuickMaterialPlaceholderText::controlImplicitBackgroundHeight() const
{
    return m_controlImplicitBackgroundHeight;
}

void QQuickMaterialPlaceholderText::setControlImplicitBackgroundHeight(qreal controlImplicitBackgroundHeight)
{
    if (qFuzzyCompare(m_controlImplicitBackgroundHeight, controlImplicitBackgroundHeight))
        return;

    m_controlImplicitBackgroundHeight = controlImplicitBackgroundHeight;
    updateFocusAnimation();
    emit controlImplicitBackgroundHeightChanged();
}

/*!
    \internal

    Exists so that we can call updateY when the control's height changes,
    which is necessary for some y position calculations.

    We don't really need it for the actual calculations, since we already
    have access to the control, from which the property comes, but
    it's simpler just to use it.
*/
qreal QQuickMaterialPlaceholderText::controlHeight() const
{
    return m_controlHeight;
}

void QQuickMaterialPlaceholderText::setControlHeight(qreal controlHeight)
{
    if (qFuzzyCompare(m_controlHeight, controlHeight))
        return;

    m_controlHeight = controlHeight;
    updateFocusAnimation();
}

qreal QQuickMaterialPlaceholderText::verticalPadding() const
{
    return m_verticalPadding;
}

void QQuickMaterialPlaceholderText::setVerticalPadding(qreal verticalPadding)
{
    if (qFuzzyCompare(m_verticalPadding, verticalPadding))
        return;

    m_verticalPadding = verticalPadding;
    updateFocusAnimation();
    emit verticalPaddingChanged();
}

void QQuickMaterialPlaceholderText::setLeftPadding(int leftPadding)
{
    if (leftPadding == m_leftPadding)
        return;

    m_leftPadding = leftPadding;
    updateFocusAnimation();
}

void QQuickMaterialPlaceholderText::setFloatingLeftPadding(int floatingLeftPadding)
{
    if (floatingLeftPadding == m_floatingLeftPadding)
        return;

    m_floatingLeftPadding = floatingLeftPadding;
    updateFocusAnimation();
}

void QQuickMaterialPlaceholderText::adjustTransformOrigin()
{
    switch (effectiveHAlign()) {
    case QQuickText::AlignLeft:
        Q_FALLTHROUGH();
    case QQuickText::AlignJustify:
        setTransformOrigin(QQuickItem::Left);
        break;
    case QQuickText::AlignRight:
        setTransformOrigin(QQuickItem::Right);
        break;
    case QQuickText::AlignHCenter:
        setTransformOrigin(QQuickItem::Center);
        break;
    }
}

void QQuickMaterialPlaceholderText::controlActiveFocusChanged()
{
    if (m_focusAnimation) {
        // Focus changes can happen before the animations finish.
        // In that case, stop the animation, which will eventually delete it.
        // Until it's deleted, we clear the pointer so that our asserts don't fail
        // for the wrong reason.
        m_focusAnimation->stop();
        m_focusAnimation.clear();
    }
    updateFocusAnimation(true);
}

void QQuickMaterialPlaceholderText::updateFocusAnimation(bool createIfNeeded)
{
    if (shouldAnimate() && (m_focusAnimation || createIfNeeded)) {
        int duration = 300;
        if (m_focusAnimation) {
            duration = m_focusAnimation->totalDuration() - m_focusAnimation->currentTime();
            m_focusAnimation->stop();
            m_focusAnimation.clear();
        }

        m_focusAnimation = new QParallelAnimationGroup(this);

        auto *yAnimation = new QPropertyAnimation(this, "y", this);
        yAnimation->setDuration(duration);
        yAnimation->setStartValue(y());
        yAnimation->setEndValue(shouldFloat() ? floatingTargetY() : normalTargetY());
        yAnimation->setEasingCurve(*animationEasingCurve);
        m_focusAnimation->addAnimation(yAnimation);

        QPropertyAnimation *xAnimation = new QPropertyAnimation(this, "x", this);
        xAnimation->setDuration(duration);
        xAnimation->setStartValue(x());
        xAnimation->setEndValue(shouldFloat() ? floatingTargetX() : normalTargetX());
        xAnimation->setEasingCurve(*animationEasingCurve);
        m_focusAnimation->addAnimation(xAnimation);

        auto *scaleAnimation = new QPropertyAnimation(this, "scale", this);
        scaleAnimation->setDuration(duration);
        scaleAnimation->setStartValue(scale());
        scaleAnimation->setEndValue(shouldFloat() ? floatingScale : 1.0);
        yAnimation->setEasingCurve(*animationEasingCurve);
        m_focusAnimation->addAnimation(scaleAnimation);

        m_focusAnimation->start(QAbstractAnimation::DeleteWhenStopped);
    } else {
        if (m_focusAnimation) {
            m_focusAnimation->stop();
            m_focusAnimation.clear();
        }
        updateY();
        updateX();
        setScale(shouldFloat() ? floatingScale : 1.0);
    }
}

void QQuickMaterialPlaceholderText::componentComplete()
{
    QQuickPlaceholderText::componentComplete();

    adjustTransformOrigin();

    m_largestHeight = implicitHeight();
    if (m_largestHeight > 0) {
        emit largestHeightChanged();
    } else {
        qmlWarning(this) << "Expected implicitHeight of placeholder text" << text()
            << "to be greater than 0 by component completion!";
    }

    updateFocusAnimation();
}

QT_END_NAMESPACE
