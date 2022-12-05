import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl

Row {
    width: 200
    height: 200

    IconImage {
        source: "qrc:/data/icons/testtheme/appointment-new.svg"
        sourceSize: Qt.size(22, 22)
    }
    Image {
        source: "qrc:/data/icons/testtheme/appointment-new.svg"
        sourceSize: Qt.size(22, 22)
    }
}
