// RootItem.qml
import QtQuick
import QtQuick.Controls.Basic

Item {
    id: root
    width: 640
    height: 480

    property double scaleFactor: 2.0
    Scale {
        id: scale
        xScale: root.scaleFactor
        yScale: root.scaleFactor
    }
    Overlay.overlay.transform: scale
}
