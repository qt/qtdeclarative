import QtQuick
import QtQuick.VectorImage

Rectangle { 
    id: topLevelItem
    width: 800
    height: 800

    ListModel {
        id: renderers
        ListElement { renderer: VectorImage.GeometryRenderer }
        ListElement { renderer: VectorImage.CurveRenderer }
    }

    Row {
        Repeater {
            model: renderers

            VectorImage {
                source: "../shared/svg/gradientxform.svg"
                preferredRendererType: renderer
            }
        }
    }
}
