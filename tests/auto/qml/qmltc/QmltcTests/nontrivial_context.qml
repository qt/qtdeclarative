import QtQuick

Item {
    id: button
    property string text: textItem.text

    Text {
        id: textItem
        Behavior on color { ColorAnimation { duration: 120 } }
    }

    MouseArea {
        id: mouse
        onClicked: function(event) {
            embedded.text = "clicked";
        }
    }

    LocallyImported {
        id: imported

        Item {
            id: embedded
            property string text: "hello, world"
        }

        HelloWorld {
            id: helloWorld
        }
    }
}
