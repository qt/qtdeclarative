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

import QtQuick 2.4
import QtQuick.Controls 2.0

AbstractBusyIndicator {
    id: control

    implicitWidth: indicator.implicitWidth + padding.left + padding.right
    implicitHeight: indicator.implicitHeight + padding.top + padding.bottom

    Accessible.role: Accessible.Indicator

    padding { top: style.padding; left: style.padding; right: style.padding; bottom: style.padding }

    indicator: Item {
        id: delegate
        implicitWidth: 48
        implicitHeight: 48
        x: (parent.width - width) / 2
        y: (parent.height - height) / 2
        width: parent.width
        height: parent.height

        opacity: control.running ? 1 : 0
        Behavior on opacity { OpacityAnimator { duration: 250 } }

        Image {
            x: (parent.width - width) / 2
            y: (parent.height - height) / 2
            width: Math.min(parent.width, parent.height)
            height: width
            source: width <= 32 ? "qrc:/images/spinner_small.png" :
                    width >= 48 ? "qrc:/images/spinner_large.png" :
                                  "qrc:/images/spinner_medium.png"

            RotationAnimator on rotation {
                duration: 800
                loops: Animation.Infinite
                from: 0
                to: 360
                running: control.visible && (control.running || delegate.opacity > 0)
            }
        }
    }
}
