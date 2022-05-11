import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ColumnLayout {
    spacing: 2

    Button {
        text: "Ok"
        Layout.preferredWidth: 80
        Layout.preferredHeight: 40
    }

    Button {
        text: ""
        Layout.preferredWidth: 106
        Layout.preferredHeight: 35
        focus: true
    }

    Button {
        text: "YES"
        flat: true
        Layout.preferredWidth: 200
        Layout.preferredHeight: 100
    }

    Button {
        text: "Button"
        highlighted: true
        enabled: false
    }

    Button {
        text: "HELLO"
        down: true
        Layout.preferredWidth: 100
        Layout.preferredHeight: 100
    }
}
