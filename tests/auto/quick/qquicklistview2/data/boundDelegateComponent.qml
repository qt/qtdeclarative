pragma ComponentBehavior: Bound

import QtQuick
Item {
    id: outer
    objectName: "outer"
    ListView {
        id: listView
        model: 1
        property string foo: "foo"
        delegate: Text {
            property var notThere: index
            objectName: listView.foo + outer.objectName + notThere
        }
    }

    ListView {
        id: listView2
        model: 1
        delegate: Text {
            required property int index
            objectName: listView.foo + outer.objectName + index
        }
    }

    Component {
        id: outerComponent
        Item {
            ListModel {
                id: listModel
                ListElement {
                    myColor: "red"
                }

                ListElement {
                    myColor: "green"
                }

                ListElement {
                    myColor: "blue"
                }
            }

            Component {
                id: innerComponent
                Rectangle {
                    objectName: model.myColor
                }
            }

            ListView {
                id: innerListView
                model: listModel
                delegate: innerComponent
            }
        }
    }
}
