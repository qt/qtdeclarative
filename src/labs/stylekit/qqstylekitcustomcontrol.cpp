// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqstylekitcustomcontrol_p.h"

QT_BEGIN_NAMESPACE

/*!
    \qmltype CustomControl
    \inqmlmodule Qt.labs.StyleKit
    \inherits ControlStyle
    \brief Defines styling for a custom control type.

    \labs
*/

/*!
    \qmlproperty int CustomControl::controlType
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
