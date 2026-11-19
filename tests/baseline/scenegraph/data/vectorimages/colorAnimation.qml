import QtQuick
import QtQuick.VectorImage

Rectangle { 
    id: topLevelItem
    width: 600
    height: 100

    ListModel {
        id: renderers
        ListElement { renderer: VectorImage.GeometryRenderer }
        ListElement { renderer: VectorImage.CurveRenderer }
    }

    Row {
        Repeater {
            model: renderers

            VectorImage {
                layer.enabled: true
                layer.samples: 4
                source: "../shared/svg/simple_animateColor.svg"
                preferredRendererType: renderer
            }
        }
    }
}
