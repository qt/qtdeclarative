// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

import QtQuick
import QtQuick.Templates as T
import QtQuick.Controls.iOS.impl
import QtQuick.Controls.impl

T.ScrollBar {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)

    visible: control.policy !== T.ScrollBar.AlwaysOff
    minimumSize: orientation === Qt.Horizontal ? height / width : width / height

    contentItem: NinePatchImage {
        width: control.availableWidth
        height: control.availableHeight

        source: IOS.url + "scrollindicator-handle"
        NinePatchImageSelector on source {
            states: [
                {"light": Application.styleHints.colorScheme === Qt.Light},
                {"dark": Application.styleHints.colorScheme === Qt.Dark},
                {"horizontal": control.horizontal},
                {"vertical": control.vertical}
            ]
        }
        opacity: 0.0
    }

    states: State {
        name: "active"
        when: control.policy === T.ScrollBar.AlwaysOn || (control.active && control.size < 1.0)
        PropertyChanges { control.contentItem.opacity: 0.75 }
    }

    transitions: Transition {
        from: "active"
        SequentialAnimation {
            PauseAnimation { duration: 450 }
            NumberAnimation { target: control.contentItem; duration: 200; property: "opacity"; to: 0.0 }
        }
    }
}
