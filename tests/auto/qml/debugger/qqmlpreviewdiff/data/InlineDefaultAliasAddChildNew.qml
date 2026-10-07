import QtQuick
import QtQuick.Layouts

Item {
    width: 400; height: 300

    component Notice: Rectangle {
        default property alias content: body.data
        Layout.fillWidth: true
        implicitHeight: body.implicitHeight + 20
        ColumnLayout {
            id: body
            anchors { left: parent.left; right: parent.right; top: parent.top; margins: 10 }
        }
    }

    ColumnLayout {
        Item { objectName: "probe" }
        anchors.fill: parent
        Notice { Text { text: "one"; color: palette.text } }
        Notice { Text { text: "two" } }
    }
}
