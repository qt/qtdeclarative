import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ColumnLayout {
    spacing: 4
    width: 200

    ProgressBar {
        value: 0
        Layout.alignment: Qt.AlignHCenter
    }

    ProgressBar {
        value: 1
        Layout.alignment: Qt.AlignHCenter
    }

    ProgressBar {
        value: 0.5
        Layout.alignment: Qt.AlignHCenter
    }

    ProgressBar {
        from: 10
        to: 20
        value: 15
        Layout.alignment: Qt.AlignHCenter
    }

}
