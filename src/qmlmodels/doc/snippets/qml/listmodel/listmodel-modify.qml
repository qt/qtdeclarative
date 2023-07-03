// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
import QtQuick

Rectangle {
    width: 200; height: 200

ListModel {
    id: fruitModel

    ListElement {
        name: "Apple"
        cost: 2.45
        attributes: [
            ListElement { description: "Core" },
            ListElement { description: "Deciduous" }
        ]
    }
    ListElement {
        name: "Orange"
        cost: 3.25
        attributes: [
            ListElement { description: "Citrus" }
        ]
    }
    ListElement {
        name: "Banana"
        cost: 1.95
        attributes: [
            ListElement { description: "Tropical" },
            ListElement { description: "Seedless" }
        ]
    }
}

//![delegate]
    Component {
        id: fruitDelegate
        Item {
            width: 200; height: 50
            Text { text: name }
            Text { text: '$' + cost; anchors.right: parent.right }

            // Double the price when clicked.
            MouseArea {
                anchors.fill: parent
                onClicked: fruitModel.setProperty(index, "cost", cost * 2)
            }
        }
    }
//![delegate]

ListView {
    width: 200; height: 200
    model: fruitModel
    delegate: fruitDelegate
}

}
