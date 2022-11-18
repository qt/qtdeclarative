import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl

Row {
    width: 200
    height: 200

    IconImage {
        name: "appointment-new"
        sourceSize: Qt.size(22, 22)
    }
    Image {
        source: "qrc:/icons/testtheme/22x22/actions/appointment-new.png"
    }
}
