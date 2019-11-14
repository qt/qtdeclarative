import QtQuick 2.0

Item {
    Component {
        id: foo
        Item {
            property real yyy: parent.progress
            Component.onCompleted: console.log(yyy)
        }
    }

    property var stuff: foo.createObject()
}
