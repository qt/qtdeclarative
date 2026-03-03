import QtQuick
import QtQuick.VectorImage

Rectangle {
    id: topLevelItem
    width: 820
    height: 820

    ListModel {
        id: files
        ListElement { src: "../shared/svg/stroking_capStyle_shapes_1.svg" }
        ListElement { src: "../shared/svg/stroking_capStyle_shapes_2.svg" }
        ListElement { src: "../shared/svg/stroking_cosmetic.svg" }
        ListElement { src: "../shared/svg/stroking_dash.svg" }
        ListElement { src: "../shared/svg/stroking_joinStyle_shapes_1.svg" }
        ListElement { src: "../shared/svg/stroking_text.svg" }
    }

    Grid {
        columns: 2
        anchors.fill: parent
        Repeater {
            model: files
            VectorImage {
                source: src
                preferredRendererType: VectorImage.CurveRenderer
                width: 400
                height: implicitHeight * width / implicitWidth
                clip: true
            }
        }
    }
}
