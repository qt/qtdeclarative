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

        //! [checkBox]
        checkBox {
            background.visible: false
            text.alignment: Qt.AlignVCenter | Qt.AlignLeft
            indicator {
                color: "transparent"
                border.width: 1
                foreground {
                    color: "transparent"
                    image.source: "qrc:/qt-project.org/imports/QtQuick/Controls/Basic/images/check.png"
                    // Hide the checkMark when the CheckBox is unchecked
                    visible: false
                }
            }
            checked {
                // Show the checkMark when the CheckBox is checked
                indicator.foreground.visible: true
            }
        }
        //! [checkBox]

        //! [up and down indicator]
        spinBox {
            text.alignment: Qt.AlignHCenter | Qt.AlignVCenter
            padding: 0
            indicator {
                implicitHeight: Style.Stretch
                color: "navy"
                opacity: 0.1
                foreground {
                    color: "transparent"
                    margins: 10
                    image.color: "navy"
                    image.fillMode: Image.PreserveAspectFit
                    image.source: "qrc:/qt-project.org/imports/QtQuick/Controls/Basic/images/arrow-indicator.png"
                    implicitWidth: 10
                    implicitHeight: 10
                    alignment: Qt.AlignCenter
                }
                down {
                    alignment: Qt.AlignLeft
                    foreground.rotation: 90
                }
                up {
                    alignment: Qt.AlignRight
                    foreground.rotation: -90
                }
            }
        }
        //! [up and down indicator]
    }

    // The rest of the file is not a part of the docs. It just implements a small
    // UI to allow testing the style from the command line using the 'qml' app.

    ScrollView {
        anchors.fill: parent
        Column {
            anchors.fill: parent
            anchors.margins: 10
            spacing: 10

            CheckBox {
                text: "CheckBox"
            }
            SpinBox {
            }
        }
    }
}
