// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls

StackView {
    id: stackView
    initialItem: page

    Component {
        id: page

        Pane {
            id: pane
            width: parent ? parent.width : 0 // TODO: fix null parent on destruction

            Column {
                spacing: 40
                width: parent.width

                Label {
                    width: parent.width
                    wrapMode: Label.Wrap
                    horizontalAlignment: Qt.AlignHCenter
                    text: "StackView provides a stack-based navigation model which can be used with a set of interlinked pages. "
                    + "Items are pushed onto the stack as the user navigates deeper into the material, and popped off again "
                    + "when he chooses to go back."
                }

                Button {
                    id: button
                    text: "Push"
                    anchors.horizontalCenter: parent.horizontalCenter
                    width: Math.max(button.implicitWidth, Math.min(button.implicitWidth * 2, pane.availableWidth / 3))
                    onClicked: stackView.push(page)
                }

                Button {
                    text: "Pop"
                    enabled: stackView.depth > 1
                    width: Math.max(button.implicitWidth, Math.min(button.implicitWidth * 2, pane.availableWidth / 3))
                    anchors.horizontalCenter: parent.horizontalCenter
                    onClicked: stackView.pop()
                }

                Label {
                    width: parent.width
                    wrapMode: Label.Wrap
                    horizontalAlignment: Qt.AlignHCenter
                    text: "Stack Depth: " + stackView.depth
                }
            }
        }
    }
}
