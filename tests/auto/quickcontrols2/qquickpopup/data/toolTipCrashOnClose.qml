// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Window
import QtQuick.Controls
import QtGraphicalEffects

Window {
    width: 640
    height: 480

    readonly property bool toolTipOpened: mouseArea.ToolTip.toolTip.opened

    Component.onCompleted: contentItem.objectName = "windowContentItem"

    // For the setOverlayParentToNull test.
    function nullifyOverlayParent() {
        Overlay.overlay.parent = null
    }

    Item {
        objectName: "outerItem"
        anchors.fill: parent

        Item {
            objectName: "innerItem"
            anchors.fill: parent

            ColorOverlay {
                objectName: "colorOverlay"
                source: parent
                anchors.fill: parent
            }

            MouseArea {
                id: mouseArea
                objectName: "mouseArea"
                anchors.fill: parent
                hoverEnabled: true

                ToolTip.visible: containsMouse
                ToolTip.text: "ToolTip text"
            }
        }
    }
}
