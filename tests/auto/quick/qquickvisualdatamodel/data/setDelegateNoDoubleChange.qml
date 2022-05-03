import QtQuick

Item {
    width: 200
    height: 200
    id: root
    property int creationCount: 0
    property bool testStarted: false

    ListModel {
        id: mymodel
        ListElement {message: "This is my alarm"}
    }

    Component {
        id: aDelegate
        Rectangle {
            color: "blue"
            width: 100
            height: 100
        }
    }

    Component {
        id: bDelegate
        Rectangle {
            color: "red"
            width: 100
            height: 100
            Text {text: model.message }
            Component.onCompleted: root.creationCount++
        }
    }


    ListView {
        width: 200
        height: 200
        id: myListView
        model: mymodel
        delegate: testStarted ? bDelegate : aDelegate
    }
}
