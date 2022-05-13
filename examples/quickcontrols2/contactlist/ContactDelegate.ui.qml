// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

ItemDelegate {
    id: delegate

    checkable: true

    contentItem: ColumnLayout {
        spacing: 10

        Label {
            text: fullName
            font.bold: true
            elide: Text.ElideRight
            Layout.fillWidth: true
        }

        GridLayout {
            id: grid
            visible: false

            columns: 2
            rowSpacing: 10
            columnSpacing: 10

            Label {
                text: qsTr("Address:")
                Layout.leftMargin: 60
            }

            Label {
                text: address
                font.bold: true
                elide: Text.ElideRight
                Layout.fillWidth: true
            }

            Label {
                text: qsTr("City:")
                Layout.leftMargin: 60
            }

            Label {
                text: city
                font.bold: true
                elide: Text.ElideRight
                Layout.fillWidth: true
            }

            Label {
                text: qsTr("Number:")
                Layout.leftMargin: 60
            }

            Label {
                text: number
                font.bold: true
                elide: Text.ElideRight
                Layout.fillWidth: true
            }
        }
    }

    states: [
        State {
            name: "expanded"
            when: delegate.checked

            PropertyChanges {
                // TODO: When Qt Design Studio supports generalized grouped properties, change to:
                //       grid.visible: true
                target: grid
                visible: true
            }
        }
    ]
}
