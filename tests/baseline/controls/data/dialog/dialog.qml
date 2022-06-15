import QtQuick
import QtQuick.Controls
import QtQuick.Layouts


RowLayout {
    width: 800
    height: 800
    spacing: 2

    ColumnLayout {
        anchors.top: parent.top
        Dialog {
            height: 200
            visible: true
            header: Text {
                id: header
                text: qsTr("I am a text item in a dialog header")
            }
            title: "I am the title"
            footer: Text {
                id: footer
                text: qsTr("I am the footer item in this header")
            }
        }
    }

    ColumnLayout {
        anchors.centerIn: parent
        Dialog {
            visible: false
            enabled: false
            modal: true
            header: Text {
                id: header2
                text: qsTr("I am a text item in dialog header2")
            }
            title: "I am the title"
            footer: Text {
                id: footer2
                text: qsTr("I am the footer item in this dialog footer2")
            }
        }
    }

    ColumnLayout {
        anchors.centerIn: parent
        Dialog {
            width: 200
            visible: true
            id: dialog
            title: "Title"
            standardButtons: Dialog.Ok | Dialog.Cancel
            onAccepted: console.log("Ok clicked")
            onRejected: console.log("Cancel clicked")
        }
    }

    ColumnLayout {
        anchors.right: parent.right
        Dialog {
            margins: 1
            height: 500
            visible: true
            enabled: false
            header: Text {
                id: header3
                text: qsTr("I am a text item in a dialog header for header3")
            }
            title: "I am the title"
            footer: Text {
                id: footer3
                text: qsTr("I am the footer item in this dialog for footer3")
            }
        }
    }
}
