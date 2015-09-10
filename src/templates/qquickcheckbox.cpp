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

#include "qquickcheckbox_p.h"

QT_BEGIN_NAMESPACE

/*!
    \qmltype CheckBox
    \inherits Checkable
    \instantiates QQuickCheckBox
    \inqmlmodule QtQuick.Controls
    \ingroup buttons
    \brief A check box control.

    CheckBox presents an option button that can be toggled on (checked) or
    off (unchecked). Check boxes are typically used to select one or more
    options from a set of options.

    \table
    \row \li \image qtquickcontrols2-checkbox-normal.png
         \li A check box in its normal state.
    \row \li \image qtquickcontrols2-checkbox-checked.png
         \li A check box that is checked.
    \row \li \image qtquickcontrols2-checkbox-focused.png
         \li A check box that has active focus.
    \row \li \image qtquickcontrols2-checkbox-disabled.png
         \li A check box that is disabled.
    \endtable

    \code
    ColumnLayout {
        CheckBox {
            checked: true
            text: qsTr("First")
        }
        CheckBox {
            text: qsTr("Second")
        }
        CheckBox {
            checked: true
            text: qsTr("Third")
        }
    }
    \endcode

    \sa {Customizing CheckBox}
*/

QQuickCheckBox::QQuickCheckBox(QQuickItem *parent) :
    QQuickCheckable(parent)
{
    setAccessibleRole(0x0000002C); //QAccessible::CheckBox
}

QT_END_NAMESPACE
