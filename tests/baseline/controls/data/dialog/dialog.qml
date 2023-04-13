import QtQuick
import QtQuick.Controls

Item {
    width: 400
    height: 200

    Dialog {
        topMargin: 20
        leftMargin: 20
        height: 100
        width: 50
        visible: true
        enabled: true
        title: qsTr("dialog 1")
    }

    Dialog {
        topMargin: 20
        leftMargin: 90
        height: 50
        width: 50
        visible: true
        enabled: false
    }

    Dialog {
        topMargin: 20
        leftMargin: 160
        height: 60
        width: 200
        visible: true
        standardButtons: Dialog.Ok | Dialog.Cancel
    }
}
