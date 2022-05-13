// Copyright (C) 2020 The Qt Company Ltd.
import QtQuick 2.15

Item {
    property real cost1: fruitModel.get(1).cost
    property string name1: fruitModel.get(1).name
    property real cost2: fruitModel.get(2).cost
    property string name2: fruitModel.get(2).name
    ListModel {
        id: fruitModel

        ListElement {
            name: "Apple"
            cost: 2.2
        }
        ListElement {
            name: "Orange"
            cost: 3
        }
        ListElement {
            name: "Banana"
            cost: 1.95
        }
    }
    Component.onCompleted: {
        console.log(fruitModel.data(fruitModel.index(1, 0), 0))
        console.log(fruitModel.get(0).name)
    }
}
