// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Fusion

ApplicationWindow {
    width: 640
    height: 480
    visible: true
    title: qsTr("Hello World")

    RowLayout {
        anchors.fill: parent

        Pane {
            Layout.fillHeight: true
            Layout.preferredWidth: parent.width * 0.5

            DropArea {
                id: outerDropArea
                anchors.fill: parent
            }

            Popup {
                id: popup
                width: 480
                height: 320
                modal: true
                focus: true
                anchors.centerIn: Overlay.overlay
                opacity: 0.5

                CheckBox {
                    id: innerDropAreaCB
                    anchors.centerIn: parent
                    text: "Inner DropArea enable"
                }

                DropArea {
                    id: innerDropArea
                    anchors.fill: parent
                    enabled: innerDropAreaCB.checked
                }
            }

            Row {
                spacing: 10
                anchors.centerIn: parent

                Button {
                    text: "Open Popup"
                    onClicked: popup.open()
                }
            }
        }

        Pane {
            Layout.fillHeight: true
            Layout.preferredWidth: parent.width * 0.5

            ColumnLayout {
                anchors.fill: parent
                CheckBox {
                    id: outerAreaInUse
                    checked: outerDropArea.containsDrag
                    text: "Outer area active" + (outerDropArea.containsDrag ? ` (${outerDropArea.drag.x}, ${outerDropArea.drag.y})` : "")
                }

                CheckBox {
                    id: popupInUse
                    checked: innerDropArea.containsDrag
                    text: "Inner area active" + (innerDropArea.containsDrag ? ` (${innerDropArea.drag.x}, ${innerDropArea.drag.y})` : "")
                }
            }
        }
    }
}
