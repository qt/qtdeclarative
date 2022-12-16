import QtQuick 2.15
import QtQuick.Controls 2.15

Item {
    width: 400; height: 400

    Drawer {
        edge: Qt.LeftEdge
        height: 200
        width: 200
        modal: true
    }
}
