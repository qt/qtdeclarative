/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Labs Templates module of the Qt Toolkit.
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

#include "qquickbutton_p.h"
#include "qquickabstractbutton_p_p.h"

#include <QtGui/qpa/qplatformtheme.h>

QT_BEGIN_NAMESPACE

/*!
    \qmltype Button
    \inherits AbstractButton
    \instantiates QQuickButton
    \inqmlmodule Qt.labs.controls
    \ingroup qtlabscontrols-buttons
    \brief A button control.

    \image qtlabscontrols-button.gif

    Button presents a push-button control that can be pushed or clicked by
    the user. Buttons are normally used to perform an action, or to answer
    a question. Typical buttons are \e OK, \e Apply, \e Cancel, \e Close,
    \e Yes, \e No, and \e Help.

    \table
    \row \li \image qtlabscontrols-button-normal.png
         \li A button in its normal state.
    \row \li \image qtlabscontrols-button-pressed.png
         \li A button that is pressed.
    \row \li \image qtlabscontrols-button-focused.png
         \li A button that has active focus.
    \row \li \image qtlabscontrols-button-disabled.png
         \li A button that is disabled.
    \endtable

    \code
    RowLayout {
        Button {
            text: "Ok"
            onClicked: model.submit()
        }
        Button {
            text: "Cancel"
            onClicked: model.revert()
        }
    }
    \endcode

    \labs

    \sa {Customizing Button}, {Button Controls}
*/

QQuickButton::QQuickButton(QQuickItem *parent) : QQuickAbstractButton(parent)
{
}

QFont QQuickButton::defaultFont() const
{
    return QQuickControlPrivate::themeFont(QPlatformTheme::PushButtonFont);
}

QT_END_NAMESPACE
