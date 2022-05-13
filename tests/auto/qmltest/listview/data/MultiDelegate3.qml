// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick 2.12
import QtQml.Models 2.12
import Qt.labs.qmlmodels 1.0

ListView {
    width: 400
    height: 400

    property var item1: QtObject {
        property string dataType: "rect"
        property color color: "red"
    }
    property var item2: QtObject {
        property string dataType: "text"
        property string text: "Hello world"
    }
    model: [ item1, item2 ]

    delegate: DelegateChooser {
        role: "dataType"
        DelegateChoice {
            roleValue: "rect"
            delegate: Rectangle {
                width: parent.width
                height: 50
                color: modelData.color
            }
        }
        DelegateChoice {
            roleValue: "text"
            delegate: Text {
                width: parent.width
                height: 50
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                text: modelData.text
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
