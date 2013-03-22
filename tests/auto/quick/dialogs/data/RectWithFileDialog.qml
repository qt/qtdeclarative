import QtQuick 2.0
import QtQuick.Dialogs 1.0

Rectangle {
    width: 1024
    height: 320
    property alias fileDialog: fileDialog
    property alias label: label
    property alias mouseArea: mouseArea

    FileDialog {
        id: fileDialog
        title: "Choose some files"
        selectMultiple: true
        nameFilters: [ "QML files (*.qml)", "All files (*)" ]
        selectedNameFilter: "QML files (*.qml)"
        onAccepted: label.text = fileDialog.filePaths
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        onClicked: fileDialog.visible = !fileDialog.visible
    }

    Text {
        id: label
        text: "Click to open a file dialog"
        wrapMode: Text.Wrap
        anchors.fill: parent
        anchors.margins: 10
    }
}
