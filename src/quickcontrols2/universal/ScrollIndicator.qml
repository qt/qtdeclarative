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
import QtQuick.Templates as T
import QtQuick.Controls.Universal

T.ScrollIndicator {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)

    contentItem: Rectangle {
        implicitWidth: 6
        implicitHeight: 6

        color: control.Universal.baseMediumLowColor
        visible: control.size < 1.0
        opacity: 0.0

        states: [
            State {
                name: "active"
                when: control.active
            }
        ]

        transitions: [
            Transition {
                to: "active"
                NumberAnimation { target: control.contentItem; property: "opacity"; to: 1.0 }
            },
            Transition {
                from: "active"
                SequentialAnimation {
                    PauseAnimation { duration: 5000 }
                    NumberAnimation { target: control.contentItem; property: "opacity"; to: 0.0 }
                }
            }
        ]
    }
}
