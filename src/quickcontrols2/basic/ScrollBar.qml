/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Quick Controls 2 module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
******************************************************************************/

import QtQuick
import QtQuick.Controls.impl
import QtQuick.Templates as T

T.ScrollBar {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)

    padding: 2
    visible: control.policy !== T.ScrollBar.AlwaysOff
    minimumSize: orientation === Qt.Horizontal ? height / width : width / height

    contentItem: Rectangle {
        implicitWidth: control.interactive ? 6 : 2
        implicitHeight: control.interactive ? 6 : 2

        radius: width / 2
        color: control.pressed ? control.palette.dark : control.palette.mid
        opacity: 0.0

        states: State {
            name: "active"
            when: control.policy === T.ScrollBar.AlwaysOn || (control.active && control.size < 1.0)
            PropertyChanges { target: control.contentItem; opacity: 0.75 }
        }

        transitions: Transition {
            from: "active"
            SequentialAnimation {
                PauseAnimation { duration: 450 }
                NumberAnimation { target: control.contentItem; duration: 200; property: "opacity"; to: 0.0 }
            }
        }
    }
}
