import QtQuick
import Test

Window {
    id: root
    width: 200
    height: 200

    property alias listView: listView

    ResetInConstructorModel {
        id: resetInConstructorModel
    }

    ListView {
        id: listView
        anchors.fill: parent
        model: resetInConstructorModel

        delegate: Rectangle {
            implicitWidth: 100
            implicitHeight: 50
            color: "olivedrab"

            required property string display
        }
    }
}
