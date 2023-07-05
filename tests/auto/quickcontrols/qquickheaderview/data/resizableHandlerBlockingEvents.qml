import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    width: 800
    height: 600

    ColumnLayout {
        anchors.fill: parent

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: mouseArea.pressed ? "tomato" : "steelblue"

            MouseArea {
                id: mouseArea
                objectName: "mouseArea"
                anchors.fill: parent
            }
        }

        HorizontalHeaderView {
            Layout.fillWidth: true
            Layout.fillHeight: true
        }
    }
}
