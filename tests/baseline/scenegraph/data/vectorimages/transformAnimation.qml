import QtQuick
import QtQuick.VectorImage

Rectangle { 
    id: topLevelItem
    width: 800
    height: 600

    ListModel {
        id: renderers
        ListElement { src: "../shared/svg/animateTransformReplace.svg" }
        ListElement { src: "../shared/svg/animateTransformSum.svg" }
        ListElement { src: "../shared/svg/animateTransformSum2.svg" }
        ListElement { src: "../shared/svg/animateTransformRemove.svg" }
        ListElement { src: "../shared/svg/animateTransformRemoveSum.svg" }
        ListElement { src: "../shared/svg/animateTransformFreezeSum.svg" }
        ListElement { src: "../shared/svg/animateTransformAll.svg" }
        ListElement { src: "../shared/svg/animateTransformUseOffset.svg" }
    }

    Grid {
        columns: 4
        spacing: 10
        anchors.fill: parent
        Repeater {
            model: renderers

            VectorImage {
                layer.enabled: true
                layer.samples: 4
                source: src
                preferredRendererType: VectorImage.CurveRenderer
            }
        }
    }
}
