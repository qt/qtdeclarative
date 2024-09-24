import QtQuick
import Test

Window {
    id: root
    width: 200
    height: 200

    property alias listView: listView

    ResettableModel {
        id: resetModel
    }

    ListView {
        id: listView
        anchors.fill: parent
        model: resetModel

        delegate: Rectangle {
            implicitWidth: 100
            implicitHeight: 50
            color: "olivedrab"

            required property string display
        }
    }
}
