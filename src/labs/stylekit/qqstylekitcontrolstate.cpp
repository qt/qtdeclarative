// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqstylekitcontrolstate_p.h"
#include "qqstylekitcontrol_p.h"

QT_BEGIN_NAMESPACE

// ************* ControlStateStyle ****************

/*!
    \qmltype ControlStateStyle
    \inqmlmodule Qt.labs.StyleKit
    \inherits ControlStyleProperties
    \brief Describes the style of a control in a given state.

    ControlStateStyle describes the style of a control in a particular state.
    A \l ControlStyle inherits ControlStateStyle, since it represents the
    \e normal state — properties set directly on a \l ControlStyle describe
    how the control looks when no other state is active. State-specific
    overrides are then set through nested states such as \l pressed,
    \l hovered, and \l checked.

    Nested states are not mutually exclusive. Multiple states can be active at
    the same time — for example, a button can be both hovered and
    pressed simultaneously. When several states are active, all matching
    state overrides are applied. If the same property is set in multiple
    active states, conflicts are resolved using the following priority
    order: \l pressed, \l hovered, \l highlighted, \l focused,
    \l checked, \l vertical. So for example \c {pressed.background.color}
    wins over \c {checked.background.color} if the control is both
    \l pressed and \l checked.

    The \l disabled state is an exception: a disabled control cannot
    be interacted with, so the \l pressed, \l hovered, \l highlighted,
    and \l focused states will not apply. However, \l disabled can
    still be combined with states like \l checked and \l vertical.

    The more deeply nested a state is, the more qualified it is.
    For example, \c {hovered.pressed.background.color} takes precedence over
    \c {hovered.background.color} when both \l hovered and \l pressed are
    active. The nesting order does not matter:
    \c {hovered.pressed} and \c {pressed.hovered} are equivalent.
    However, if both forms are used at the same time, which one wins is
    undefined.

    Deeper nesting of states can also be used to resolve conflicts. If the same property
    is set in both \l hovered and \l checked, the priority order means
    the \l hovered value wins. If you would rather have the checked value win, or
    use an altogether different value in that situation, you can override
    the property in \c {hovered.checked}, which then takes precedence over both.

    The following snippet shows how to style a button differently
    depending on its state:

    \snippet ControlStateStyle_states.qml States

    \labs

    \sa ControlStyle, DelegateStyle,
        {qtlabsstylekit-fallbackstyle.html}{FallbackStyle Reference}
*/

/*!
    \qmlproperty ControlStateStyle ControlStateStyle::pressed

    Style overrides applied when the control is \l {StyleReader::}{pressed}.

    \sa hovered, highlighted, focused, checked, vertical, disabled, {StyleReader::pressed}{StyleReader.pressed}.
*/

/*!
    \qmlproperty ControlStateStyle ControlStateStyle::hovered

    Style overrides applied when the control is \l {StyleReader::}{hovered}.

    \sa pressed, highlighted, focused, checked, vertical, disabled, {StyleReader::hovered}{StyleReader.hovered}.
*/

/*!
    \qmlproperty ControlStateStyle ControlStateStyle::highlighted

    Style overrides applied when the control is \l {StyleReader::}{highlighted}.

    \sa pressed, hovered, focused, checked, vertical, disabled, {StyleReader::highlighted}{StyleReader.highlighted}.
*/

/*!
    \qmlproperty ControlStateStyle ControlStateStyle::focused

    Style overrides applied when the control is \l {StyleReader::}{focused}.

    \sa pressed, hovered, highlighted, checked, vertical, disabled, {StyleReader::focused}{StyleReader.focused}.
*/

/*!
    \qmlproperty ControlStateStyle ControlStateStyle::checked

    Style overrides applied when the control is \l {StyleReader::}{checked}.

    \sa pressed, hovered, highlighted, focused, vertical, disabled, {StyleReader::checked}{StyleReader.checked}.
*/

/*!
    \qmlproperty ControlStateStyle ControlStateStyle::vertical

    Style overrides applied when the control is \l {StyleReader::}{vertical}
    (e.g. a vertical \l Slider or \l ScrollBar).

    \sa pressed, hovered, highlighted, focused, checked, disabled, {StyleReader::vertical}{StyleReader.vertical}.
*/

/*!
    \qmlproperty ControlStateStyle ControlStateStyle::disabled

    Style overrides applied when the control is \l {StyleReader::}{disabled}.

    A disabled control cannot be interacted with, so \l pressed,
    \l hovered, \l highlighted, and \l focused will not be applied
    at the same time as disabled.

    \sa pressed, hovered, highlighted, focused, checked, vertical, {StyleReader::disabled}{StyleReader.disabled}.
*/

QQStyleKitControlState::QQStyleKitControlState(QObject *parent)
    : QQStyleKitControlProperties(QQSK::PropertyGroup::Control, parent)
{
}

QQStyleKitControl *QQStyleKitControlState::control() const
{
    if (m_nestedState == QQSK::StateFlag::Normal) {
        Q_ASSERT(qobject_cast<const QQStyleKitControl *>(this));
        auto *self = const_cast<QQStyleKitControlState *>(this);
        return static_cast<QQStyleKitControl *>(self);
    }
    Q_ASSERT(qobject_cast<const QQStyleKitControl *>(parent()));
    return static_cast<QQStyleKitControl *>(parent());
}

QQStyleKitControlState *QQStyleKitControlState::lazyCreateState(QQSK::StateFlag state) const
{
    if (m_nestedStateObjects.contains(state))
        return m_nestedStateObjects.value(state);

    QQStyleKitControlState *stateObj = new QQStyleKitControlState(control());
    stateObj->m_nestedState = m_nestedState;
    stateObj->m_nestedState.setFlag(state);
    stateObj->m_nestedState.setFlag(QQSK::StateFlag::Normal, false);

    auto *self = const_cast<QQStyleKitControlState *>(this);
    self->m_nestedStateObjects.insert(state, stateObj);

    return stateObj;
}

QQStyleKitControlState *QQStyleKitControlState::pressed() const
{
    return lazyCreateState(QQSK::StateFlag::Pressed);
}

QQStyleKitControlState *QQStyleKitControlState::hovered() const
{
    return lazyCreateState(QQSK::StateFlag::Hovered);
}

QQStyleKitControlState *QQStyleKitControlState::highlighted() const
{
    return lazyCreateState(QQSK::StateFlag::Highlighted);
}

QQStyleKitControlState *QQStyleKitControlState::focused() const
{
    return lazyCreateState(QQSK::StateFlag::Focused);
}

QQStyleKitControlState *QQStyleKitControlState::checked() const
{
    return lazyCreateState(QQSK::StateFlag::Checked);
}

QQStyleKitControlState *QQStyleKitControlState::vertical() const
{
    return lazyCreateState(QQSK::StateFlag::Vertical);
}

QQStyleKitControlState *QQStyleKitControlState::disabled() const
{
    return lazyCreateState(QQSK::StateFlag::Disabled);
}

QT_END_NAMESPACE

#include "moc_qqstylekitcontrolstate_p.cpp"
