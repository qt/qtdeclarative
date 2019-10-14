import QtQuick 2.14

Item {
    width: 400
    height: 200

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
            required property int index
            required property string name
            required property string place
            height: 100
            width: 100
            Text {
                text: myDelegate.name + " lives in " + myDelegate.place + myDelegate.index
                font.pointSize: 16
                anchors.fill: myDelegate

                Component.onCompleted: () => {console.info(myDelegate.name+myDelegate.place+myDelegate.index)}
            }
        }
    }

    PathView {
        anchors.fill: parent
        model: myModel
        delegate: delegateComponent
        path: Path {
            startX: 120; startY: 100
            PathQuad { x: 120; y: 25; controlX: 260; controlY: 75 }
            PathQuad { x: 120; y: 100; controlX: -20; controlY: 75 }
        }
    }

}
