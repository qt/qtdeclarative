import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ColumnLayout {
    GroupBox {
        title: "Title 1"
        label: CheckBox {
            text: "I am CheckBox label in a GroupBox"
            checked: true
        }
    }

    GroupBox {
        title: "Title 2"
        label: Label {
            text: "I am Text label in GroupBox"
        }
    }

    GroupBox {
        title: "Title"
        ColumnLayout {
            anchors.fill: parent
            CheckBox { text: "E-mail" }
            CheckBox { text: "Calendar" }
            CheckBox { text: "Contacts" }
        }
    }
}
