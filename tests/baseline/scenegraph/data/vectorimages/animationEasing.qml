import QtQuick
import QtQuick.VectorImage

Rectangle { 
    id: topLevelItem
    width: 400
    height: 350

    ListModel {
        id: files
        ListElement { src: "../shared/svg/animationEasing.svg" }
    }

    Grid {
        anchors.fill: parent
        Repeater {
            model: files
            VectorImage {
                source: src
                preferredRendererType: VectorImage.CurveRenderer
            }
        }
    }
}
