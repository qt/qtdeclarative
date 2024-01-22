import QtQml

QtObject {
    id: root
    objectName: button.objectName

    property QtObject b: QtObject {
        objectName: "button"
        id: button
        signal clicked
    }

    property Connections c: Connections {
        id: connections
        target: null
        function onClicked() { button.destroy(); }
    }

    property Timer t: Timer {
       interval: 10
       running: true
       onTriggered: {
           root.objectName = connections.target.objectName
       }
    }

    Component.onCompleted: {
        connections.target = button;
        button.clicked()
    }
}
