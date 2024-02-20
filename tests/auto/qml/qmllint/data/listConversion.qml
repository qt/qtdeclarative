import QtQuick
import QtQuick.Controls

Item {
    ButtonGroup {
        buttons: column.children
    }

    Column {
        id: column

        RadioButton {
            checked: true
            text: qsTr("DAB")
        }

        RadioButton {
            text: qsTr("FM")
        }

        RadioButton {
            text: qsTr("AM")
        }
    }
}
