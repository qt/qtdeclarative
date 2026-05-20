import QtQuick
import QtQuick.Shapes
import QtQuick.Shapes.DesignHelpers

Window {
    width: 100
    height: 100

    property alias theShape: theShape

    RegularPolygonShape {
        id: theShape
        width: 100
        height: 100
        anchors.centerIn: parent

        fillItem: Image {
            objectName: "fillItem"
            visible: false
            source: "qrc:/data/col320x480.jpg"
            parent: theShape
        }
    }
}
