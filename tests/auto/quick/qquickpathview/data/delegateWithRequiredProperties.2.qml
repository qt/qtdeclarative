import QtQuick 2.14

Item {
    id: root
    width: 800
    height: 600
    property bool working: false

    ListModel {
        id: myModel
        ListElement {
            name: "Bill Jones"
            place: "Berlin"
        }
        ListElement {
            name: "Jane Doe"
            place: "Oslo"
        }
        ListElement {
            name: "John Smith"
            place: "Oulo"
        }
    }

    Component {
        id: delegateComponent
        Rectangle {
            id: myDelegate
            height: 50
            width: 50
        required property string name
        required property int index
        onNameChanged: () => {if (myDelegate.name === "You-know-who") root.working = true}
            Text {
                text: myDelegate.name
                font.pointSize: 10
                anchors.fill: myDelegate
            }
        }
    }

    PathView {
        anchors.fill: parent
        model: myModel
        delegate: delegateComponent
        path: Path {
            startX: 80; startY: 100
            PathQuad { x: 120; y: 25; controlX: 260; controlY: 75 }
            PathQuad { x: 140; y: 100; controlX: -20; controlY: 75 }
        }
    }
    Timer {
        interval: 1
        running: true
        repeat: false
        onTriggered: () => { myModel.setProperty(1, "name", "You-know-who"); }
    }

}
