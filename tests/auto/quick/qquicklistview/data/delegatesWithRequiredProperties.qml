import QtQuick 2.12
import Qt.fruit 1.0

Rectangle {
    id: root
    required property bool useCpp
    width: 200; height: 200


    ListModel {
        id: fruitModel

        ListElement {
            name: "Apple"
            cost: 2
        }
        ListElement {
            name: "Orange"
            cost: 3
        }
        ListElement {
            name: "Banana"
            cost: 1
        }
    }


    Component {
        id: fruitDelegate
        Row {
            id: row
            spacing: 10
            required property string name
            required property int cost
            Text { text: row.name }
            Text { text: '$' + row.cost }
            Component.onCompleted: () => { console.debug(row.name+row.cost) };
        }
    }

    ListView {
        anchors.fill: parent
        model: root.useCpp ? FruitModelCpp : fruitModel
        delegate: fruitDelegate
    }

}
