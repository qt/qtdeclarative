pragma ComponentBehavior: Bound

import QtQuick
import Qt.labs.qmlmodels

Item {
    id: outer
    objectName: "outer"
    TableView {
        id: tableView
        width: 10
        height: 10
        model: 1
        property string foo: "foo"
        delegate: Text {
            property var notThere: index
            text: "."
            objectName: tableView.foo + outer.objectName + notThere
        }
    }

    TableView {
        id: tableView2
        width: 10
        height: 10
        model: 1
        delegate: Text {
            required property int index
            text: "."
            objectName: tableView.foo + outer.objectName + index
        }
    }

    Component {
        id: outerComponent
        Item {
            TableModel {
                id: tableModel
                TableModelColumn { display: "color" }
                TableModelColumn { display: "amount" }
                rows: [
                    { color: "red",   amount: 1 },
                    { color: "green", amount: 3 },
                    { color: "blue",  amount: 8 },
                ]

            }

            Component {
                id: innerComponent
                Rectangle {
                    implicitWidth: 1
                    implicitHeight: 1
                    objectName: model.myColor
                }
            }

            TableView {
                width: 10
                height: 10
                id: innerTableView
                model: tableModel
                delegate: innerComponent
            }
        }
    }
}
