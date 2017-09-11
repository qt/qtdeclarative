import QtQuick 2.10
import QtQuick.Controls 2.3
import QtQuick.Controls.impl 2.3

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
