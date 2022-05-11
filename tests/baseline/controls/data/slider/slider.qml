import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ColumnLayout {
    spacing: 4
    width: 200

    Slider {
        from: 1
        value: 25
        to: 100
        stepSize: 10
        Layout.alignment: Qt.AlignHCenter
    }

    Slider {
        orientation: Qt.Vertical
        from: 1
        to: 10
        Layout.alignment: Qt.AlignHCenter
    }

    Slider {
        from: 1
        value: 25
        to: 100
        Layout.alignment: Qt.AlignHCenter
        focus: true
    }


    Slider {
        from: 1
        value: 25
        to: 100
        stepSize: 20
        Layout.alignment: Qt.AlignHCenter
    }

    Slider {
        from: 1
        value: 100
        to: 100
        enabled: false
        Layout.alignment: Qt.AlignHCenter
    }
}
