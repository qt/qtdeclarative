// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick 2.12
import QtQml.Models 2.12
import Qt.labs.qmlmodels 1.0

ListView {
    width: 400
    height: 400
    model: ListModel {
        ListElement { dataType: "rect"; color: "red" }
        ListElement { dataType: "image"; source: "logo.png" }
        ListElement { dataType: "text"; text: "Hello" }
        ListElement { dataType: "text"; text: "World" }
        ListElement { dataType: "rect"; color: "green" }
        ListElement { dataType: "image"; source: "logo.png" }
        ListElement { dataType: "rect"; color: "blue" }
        ListElement { dataType: "" }
    }

    delegate: DelegateChooser {
        role: "dataType"
        DelegateChoice {
            roleValue: "image"
            delegate: Image {
                width: parent.width
                height: 50
                fillMode: Image.PreserveAspectFit
                source: model.source
            }
        }
        DelegateChoice {
            roleValue: "rect"
            delegate: Rectangle {
                width: parent.width
                height: 50
                color: model.color
            }
        }
        DelegateChoice {
            roleValue: "text"
            delegate: Text {
                width: parent.width
                height: 50
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                text: model.text
            }
        }

        DelegateChoice {
            delegate: Item {
                    width: parent.width
                    height: 50
                }
        }
    }
}
