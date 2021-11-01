import QtQuick 2.8

Item {
    id: root
    width: 400
    height: 400

    objectName: delayed2.objectName

    property int changeCount: 0
    property int changeCount2: 0

    property string text1
    property string text2

    function updateText() {
        text1 = "Hello"
        text2 = "World"
    }

    function resetText() {
        text1 = "";
        text2 = "";
    }

    property bool delayed: true;

    Text {
        anchors.centerIn: parent
        Binding on text {
            value: text1 + " " + text2
            delayed: root.delayed
        }
        onTextChanged: ++changeCount
    }

    Text {
        id: child
        anchors.centerIn: parent
        Binding {
            id: delayed2
            objectName: "c: " + root.x
            root {
                x: 10
                y: 20
            }
            child.text: root.text1 + " " + root.text2
            delayed: root.delayed
        }
        onTextChanged: ++root.changeCount2
    }
}

