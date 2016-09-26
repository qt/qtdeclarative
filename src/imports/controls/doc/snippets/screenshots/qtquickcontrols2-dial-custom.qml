/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the documentation of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:FDL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Free Documentation License Usage
** Alternatively, this file may be used under the terms of the GNU Free
** Documentation License version 1.3 as published by the Free Software
** Foundation and appearing in the file included in the packaging of
** this file. Please review the following information to ensure
** the GNU Free Documentation License version 1.3 requirements
** will be met: http://www.gnu.org/copyleft/fdl.html.
** $QT_END_LICENSE$
**
****************************************************************************/

//! [file]
import QtQuick 2.6
import QtQuick.Controls 2.0

Dial {
    id: control
    background: Rectangle {
        x: control.width / 2 - width / 2
        y: control.height / 2 - height / 2
        width: Math.max(64, Math.min(control.width, control.height))
        height: width
        color: "transparent"
        radius: width / 2
        border.color: control.pressed ? "#17a81a" : "#21be2b"
        opacity: control.enabled ? 1 : 0.3
    }

    handle: Rectangle {
        id: handleItem
        x: background.x + background.width / 2 - handle.width / 2
        y: background.y + background.height / 2 - handle.height / 2
        width: 16
        height: 16
        color: control.pressed ? "#17a81a" : "#21be2b"
        radius: 8
        antialiasing: true
        opacity: control.enabled ? 1 : 0.3
        transform: [
            Translate {
                y: -Math.min(background.width, background.height) * 0.4 + handle.height / 2
            },
            Rotation {
                angle: control.angle
                origin.x: handle.width / 2
                origin.y: handle.height / 2
            }
        ]
    }
}
//! [file]
