/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Quick dial1s 2 module of the Qt Toolkit.
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

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic.impl
import QtQuick.Layouts

ControlContainer {
    id: container
    title: "Dials"

    Row {
        spacing: container.rowSpacing

        Dial {
            id: dial1
            width: 50
            height: 50
            from: 0
            to: 10
            value: 5

            background: DialImpl {
                implicitWidth: 184
                implicitHeight: 184
                color: "darkgray"
                progress: dial1.position
                opacity: dial1.enabled ? 1 : 0.3
            }

            handle: ColorImage {
                x: dial1.background.x + dial1.background.width / 2 - width / 2
                y: dial1.background.y + dial1.background.height / 2 - height / 2
                width: 14
                height: 10
                color: "green"
                source: "qrc:/qt-project.org/imports/QtQuick/Controls/Basic/images/dial-indicator.png"
                antialiasing: true
                opacity: dial1.enabled ? 1 : 0.3
                transform: [
                    Translate {
                        y: -Math.min(dial1.background.width, dial1.background.height) * 0.4 + dial1.handle.height / 2
                    },
                    Rotation {
                        angle: dial1.angle
                        origin.x: dial1.handle.width / 2
                        origin.y: dial1.handle.height / 2
                    }
                ]
            }
        }
    }
}
