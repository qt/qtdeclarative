// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQml.Models
import QtQuick.Layouts
import Qt.labs.qmlmodels
import shared

Rectangle {
    visible: true
    width: 640
    height: 640

    ListModel {
        id: listModel
        ListElement { dataType: "rect"; color: "green" }
        ListElement { dataType: "image" }
        ListElement { dataType: "rect"; color: "green" }
        ListElement { dataType: "image" }
        ListElement { dataType: "rect"; color: "blue" }
        ListElement { dataType: "rect"; color: "blue" }
        ListElement { dataType: "rect"; color: "blue" }
        ListElement { dataType: "rect"; color: "blue" }
        ListElement { dataType: "rect"; color: "blue" }
        ListElement { dataType: "rect"; color: "blue" }
    }

    ListModel {
        id: listModel2
        ListElement { dataType: "rect"; color: "blue" }
        ListElement { dataType: "rect"; color: "blue" }
        ListElement { dataType: "rect"; color: "green" }
        ListElement { dataType: "image" }
        ListElement { dataType: "rect"; color: "green" }
        ListElement { dataType: "image" }
        ListElement { dataType: "rect"; color: "blue" }
        ListElement { dataType: "rect"; color: "lightsteelblue" }
        ListElement { dataType: "rect"; color: "fuchsia" }
        ListElement { dataType: "rect"; color: "lime" }
    }

    DelegateChooser {
        id: fancyDelegate
        role: "dataType"
        DelegateChoice {
            roleValue: "rect"
            delegate: DelegateChooser {
                DelegateChoice {
                    row: 0
                    Rectangle {
                        width: parent.width
                        height: 50
                        color: "red"
                        border.color: "black"
                        border.width: 1
                    }
                }
                DelegateChoice {
                    Rectangle {
                        width: parent.width
                        height: 50
                        color: model.color
                        border.color: "black"
                        border.width: 1
                    }
                }
            }
        }
        DelegateChoice {
            roleValue: "image"
            delegate: Image {
                width: parent.width
                height: 100
                source: Images.qtLogo
                fillMode: Image.PreserveAspectFit
            }
        }
    }

    Item {
        anchors.fill: parent
        id: ite
        RowLayout {
            ListView {
                Layout.preferredHeight: ite.height
                Layout.preferredWidth: ite.width * 0.5
                model: listModel
                delegate: fancyDelegate
            }
            ListView {
                Layout.preferredHeight: ite.height
                Layout.preferredWidth: ite.width * 0.5
                model: listModel2
                delegate: fancyDelegate
            }
        }
    }
}
