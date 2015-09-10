/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Quick Controls module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL3$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or later as published by the Free
** Software Foundation and appearing in the file LICENSE.GPL included in
** the packaging of this file. Please review the following information to
** ensure the GNU General Public License version 2.0 requirements will be
** met: http://www.gnu.org/licenses/gpl-2.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qquicktogglebutton_p.h"

QT_BEGIN_NAMESPACE

/*!
    \qmltype ToggleButton
    \inherits Checkable
    \instantiates QQuickToggleButton
    \inqmlmodule QtQuick.Controls
    \ingroup buttons
    \brief A toggle button control.

    ToggleButton is an option button that can be dragged or toggled on
    (checked) or off (unchecked). ToggleButtons are typically used to
    select between two states.

    \table
    \row \li \image qtquickcontrols2-togglebutton-normal.png
         \li A toggle button in its normal state.
    \row \li \image qtquickcontrols2-togglebutton-checked.png
         \li A toggle button that is checked.
    \row \li \image qtquickcontrols2-togglebutton-focused.png
         \li A toggle button that has active focus.
    \row \li \image qtquickcontrols2-togglebutton-disabled.png
         \li A toggle button that is disabled.
    \endtable

    \code
    ColumnLayout {
        ToggleButton {
            text: qsTr("Wi-Fi")
        }
        ToggleButton {
            text: qsTr("Bluetooth")
        }
    }
    \endcode

    \sa {Customizing ToggleButton}
*/

QQuickToggleButton::QQuickToggleButton(QQuickItem *parent) :
    QQuickSwitch(parent)
{
    setAccessibleRole(0x0000002B); //QAccessible::Button
}

QT_END_NAMESPACE
