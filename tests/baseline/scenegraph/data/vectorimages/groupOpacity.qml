import QtQuick
import QtQuick.VectorImage

Rectangle{
    id: topLevelItem
    width: 800
    height: 400

    ListModel {
        id: files
        ListElement { src: "../shared/svg/opacity_group_1.svg" }
        ListElement { src: "../shared/svg/opacity_group_2.svg" }
    }

    Grid {
        spacing: 10
        columns: 2
        anchors.fill: parent
        Repeater {
            model: files

            VectorImage {
                source: src
                preferredRendererType: VectorImage.CurveRenderer
                width: 400
                height: 400
                fillMode: VectorImage.PreserveAspectFit
            }
        }
    }
}
