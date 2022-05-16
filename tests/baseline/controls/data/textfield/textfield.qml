import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ColumnLayout {
    spacing: 2
    width: 200

    TextField {
        placeholderText: qsTr("Enter text")
        enabled: false
    }

    TextField {
        placeholderText: qsTr("Enter text")
        placeholderTextColor: "red"
    }

    TextField {
        placeholderText: qsTr("Enter text")
        focus: true
    }

    TextField {
        placeholderText: qsTr("Enter text")
        LayoutMirroring.enabled: true
    }

    TextField {
        text: qsTr("Lorem ipsum dolor sit amet, consectetur adipiscing elit,"
                   + "sed do eiusmod tempor incididunt utlabore et dolore magna"
                   + "aliqua.Ut enim ad minim veniam, quis nostrud exercitation"
                   + "ullamco laboris nisi ut aliquip ex ea commodo consequat.")
    }
}
