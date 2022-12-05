import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    width: 400
    height: 400

    Dialog {
        id: parentDialog
        title: "Parent Dialog"
        objectName: "parentDialog"
        width: 300
        height: 300
        anchors.centerIn: parent

        z: 1 // deliberately changing the z order here

        Dialog {
            id: subDialog
            title: "Sub Dialog"
            objectName: "subDialog"

            width: 200
            height: 200
            anchors.centerIn: parent
        }
    }
}
