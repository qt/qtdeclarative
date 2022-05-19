import QtQuick
import QtQuick.Controls

Column {
    spacing: 10

    DialogButtonBox {
        standardButtons: DialogButtonBox.Ok | DialogButtonBox.Cancel
    }

    DialogButtonBox {
        standardButtons: DialogButtonBox.Yes
    }

    DialogButtonBox {
        enabled: false
        standardButtons: DialogButtonBox.Ok | DialogButtonBox.Cancel
    }

    DialogButtonBox {
        LayoutMirroring.enabled: true
        standardButtons: DialogButtonBox.Ok | DialogButtonBox.Cancel
    }

    DialogButtonBox {
        width: parent.width
        alignment: Qt.AlignLeft
        standardButtons: DialogButtonBox.Ok | DialogButtonBox.Cancel
    }

    DialogButtonBox {
        width: parent.width
        alignment: Qt.AlignCenter
        standardButtons: DialogButtonBox.Ok | DialogButtonBox.Cancel
    }

    DialogButtonBox {
        width: parent.width
        alignment: Qt.AlignRight
        standardButtons: DialogButtonBox.Ok | DialogButtonBox.Cancel
    }

    DialogButtonBox {
        buttonLayout: DialogButtonBox.WinLayout
        standardButtons: DialogButtonBox.Ok | DialogButtonBox.Cancel
    }

    DialogButtonBox {
        buttonLayout: DialogButtonBox.MacLayout
        standardButtons: DialogButtonBox.Ok | DialogButtonBox.Cancel
    }

    DialogButtonBox {
        buttonLayout: DialogButtonBox.AndroidLayout
        standardButtons: DialogButtonBox.Ok | DialogButtonBox.Cancel
    }

    DialogButtonBox {
        Button {
            text: "Save"
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
        }
        Button {
            text: "Close"
            DialogButtonBox.buttonRole: DialogButtonBox.DestructiveRole
            enabled: false
        }
        Button {
            text: "Cancel"
            DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
        }
    }
}
