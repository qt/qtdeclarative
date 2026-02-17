// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import Qt.labs.StyleKit

ApplicationWindow {
    id: app
    width: 1024
    height: 800
    visible: true

    StyleKit.style: Style {
        //! [frame with variation]
        frame {
            variations: StyleVariation {
                button.background {
                    radius: 0
                    color: palette.accent
                }
            }
        }
        //! [frame with variation]

        //! [groupbox without variation]
        groupBox {
            // groupBox falls back to frame. Therefore, if the varations set on a
            // frame is not wanted on a groupBox, just override it and set it back to [].
            variations: []
        }
        //! [groupbox without variation]
    }

    // The rest of the file is not a part of the docs, it just creates a simple
    // UI to test the doc style from the command line: qml ControlsSnippets.qml

    ScrollView {
        anchors.fill: parent
        Column {
            anchors.fill: parent
            anchors.margins: 10
            spacing: 10

            Button {
                text: "Button"
            }

            Frame {
                width: 100
                height: 100
                clip: true
                Flickable {
                    anchors.fill: parent
                    contentWidth: 200
                    contentHeight: 200
                    ScrollIndicator.vertical: ScrollIndicator {}
                    ScrollIndicator.horizontal: ScrollIndicator {}
                    Button {
                        text: "Button inside Frame"
                    }
                }
            }
        }
    }
}
