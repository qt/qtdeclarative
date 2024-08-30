import QtQuick
import QtQuick.VectorImage

Rectangle{
    id: topLevelItem
    width: 200
    height: 200

    VectorImage {
        anchors.fill: parent
        source: "../shared/svg/circle.svg"
    }
}
