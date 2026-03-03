// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import Qt.labs.StyleKit

ApplicationWindow {
    id: app
    width: 1024
    height: 800
    visible: true

    StyleKit.style:
    //! [frame with variation]
    Style {
        frame {
            variations: StyleVariation {
                button {
                    text.color: "ghostwhite"
                    background.border.width: 0
                    background.color: "slategrey"
                }
            }
        }

        groupBox {
            // groupBox falls back to frame. Therefore, if the varations set on a
            // frame is not wanted on a groupBox, just override it and set it back to [].
            variations: []
        }
    }
    //! [frame with variation]

    // The rest of the file is not a part of the docs. It just implements a small
    // UI to allow testing the style from the command line using the 'qml' app.

    ScrollView {
        anchors.fill: parent
        Column {
            anchors.fill: parent
            anchors.margins: 10
            spacing: 10

            Button {
                text: "Button outside"
            }

            Frame {
                width: 400
                Row {
                    Button {
                        text: "Button inside"
                    }
                }
            }
        }
    }
}
