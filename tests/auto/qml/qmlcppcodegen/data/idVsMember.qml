import QtQuick
import QtQuick.Controls

Control {
    anchors.fill: parent
    objectName: contentItem.text

    MyObject {
        id: my
        foo: foo
    }

    property Item myprop: Item {
        objectName: "blub"
        id: foo
    }

    contentItem: Text {
        text: my.foo + ""
    }
}
