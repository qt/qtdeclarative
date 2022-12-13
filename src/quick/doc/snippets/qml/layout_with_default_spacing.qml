// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

//! [layout-with-default-spacing]
ApplicationWindow {
    id: root
    width: 300
    height: 300
    visible: true

    RowLayout {
        anchors.fill: parent
        ColumnLayout {
            Rectangle {
                color: "tomato";
                Layout.fillWidth: true
                Layout.fillHeight: true
            }
            RowLayout {
                Rectangle {
                    color: "navajowhite"
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                }
                Rectangle {
                    color: "darkseagreen"
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                }
            }
        }
        ColumnLayout {
            Rectangle {
                color: "lightpink"
                Layout.fillWidth: true
                Layout.fillHeight: true
            }
            Rectangle {
                color: "slategray"
                Layout.fillWidth: true
                Layout.fillHeight: true
            }
            Rectangle {
                color: "lightskyblue"
                Layout.fillWidth: true
                Layout.fillHeight: true
            }
        }
    }
}
//! [layout-with-default-spacing]

