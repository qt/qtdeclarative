import QtQuick

Item {
    property QtObject rectProperty: Rectangle {
        id: rectangle
        objectName: "rectangle"
    }

    Row {
        id: row
        objectName: "row"

        Rectangle {
            Text {
                id: textInRectangle
                objectName: "textInRectangle"
            }
        }

        property list<QtObject> listOfObjects: [
            Item {
                id: itemInList
                objectName: "itemInList"

                property QtObject foobarProperty: QtObject { id: foobar; objectName: "foobar" }
            },
            QtObject {
                id: objectInList
                objectName: "objectInList"
            }
        ]

        Item {
            id: item
            objectName: "item"
        }
    }

    property QtObject gridProperty: GridView {
        id: gridView
        objectName: "gridView"
    }

    TableView {
        id: tableView
        objectName: "tableView"

        property Component before: Component {
            id: beforeDelegate
            Text {
                id: beforeDelegateText
                objectName: "beforeDelegateText"
            }
        }
        Component {
            id: beforeDelegateDefaultProperty
            Text {
                id: beforeDelegateDefaultPropertyText
                objectName: "beforeDelegateDefaultPropertyText"
            }
        }

        delegate: Rectangle {
            id: delegateRect
            objectName: "delegateRect"
        }

        Component {
            id: afterDelegateDefaultProperty
            Text {
                id: afterDelegateDefaultPropertyText
                objectName: "afterDelegateDefaultPropertyText"
            }
        }
        property Component after: Component {
            id: afterDelegate
            Text {
                id: afterDelegateText
                objectName: "afterDelegateText"
            }
        }
    }

    property QtObject explicitCompProperty: Component {
        id: explicitComponent
        Text {
            id: explicitText
            objectName: "explicitText"
        }
    }

    property QtObject sentinelProperty: QtObject {
        id: sentinel
        objectName: "sentinel"
    }
}
