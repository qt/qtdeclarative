import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root
    ColumnLayout {
        anchors.fill: parent
        ComboBox {
        }
        Button {
            text: "Button"
        }
        CheckBox {
            text: "CheckBox"
        }
        RadioButton {
            text: "RadioButton"
        }
        TextField {
        }
    }
}
