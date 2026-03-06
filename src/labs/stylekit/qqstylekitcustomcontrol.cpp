// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqstylekitcustomcontrol_p.h"

QT_BEGIN_NAMESPACE

/*!
    \qmltype CustomControl
    \inqmlmodule Qt.labs.StyleKit
    \inherits ControlStyle
    \brief Defines styling for a custom (non-built-in) control.

    CustomControl allows you to define and style controls that are not part of
    \l {Qt Quick Controls}. This is convenient if you have additional controls
    or components that should be styled according to the active \l Style and \l Theme.

    Like the built-in control types (such as
    \l {AbstractStylableControls::}{abstractButton},
    \l {AbstractStylableControls::}{pane}, and
    \l {AbstractStylableControls::}{slider}), CustomControl inherits
    \l ControlStyle. Unlike built-in types, which are implicitly connected to
    their control type, a CustomControl requires \l controlType to be set
    explicitly. Apart from that, they work exactly the same way.

    A \l Style or \l Theme can define as many custom controls as needed, and a
    CustomControl in a Theme can have the same \l controlType as one in the \l Style.
    That is no different from, for example, a \l {AbstractStylableControls::}{slider}
    being styled by both the \l Style and the \l Theme. The fallback logic is the same.

    Any style properties not set on a CustomControl fall back to those set
    on a \l {AbstractStylableControls::}{control}.

    The following snippet shows how to define styling for a custom control:

    \snippet CustomControlSnippets.qml custom control style

    And the following snippet shows an example on how to implement a custom
    control that uses it:

    \snippet CustomControlSnippets.qml custom control

    \labs

    \sa StyleReader, ControlStyle, Style
*/

/*!
    \qmlproperty int CustomControl::controlType

    A unique integer identifying this custom control type. Set the same value
    on the \l {StyleReader::controlType}{StyleReader.controlType} in your
    custom control implementation to connect it to this style definition.

    Custom control types must be in the range \c 0 to \c 100000.

    \sa {StyleReader::controlType}{StyleReader.controlType}
*/

using namespace Qt::StringLiterals;

QQStyleKitCustomControl::QQStyleKitCustomControl(QObject *parent)
    : QQStyleKitControl(parent)
{
}

int QQStyleKitCustomControl::controlType() const
{
    return m_controlType;
}

void QQStyleKitCustomControl::setControlType(int controlType)
{
    if (m_controlType == controlType)
        return;

    m_controlType = controlType;

    emit controlTypeChanged();
}

QT_END_NAMESPACE

#include "moc_qqstylekitcustomcontrol_p.cpp"
