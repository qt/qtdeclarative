import QtQuick
import QtQuick.VectorImage
import Qt.labs.folderlistmodel

Rectangle{
    id: topLevelItem
    width: 800
    height: 600

    Grid {
        columns: 2
        anchors.fill: parent
        Repeater {
            model: FolderListModel {
                folder: Qt.resolvedUrl("../shared/svg_12_testsuite/")
                nameFilters: [ "shapes-rect*.svg"]
            }

            VectorImage {
                width: 400
                height: implicitHeight * width / implicitWidth
                source: fileUrl
                clip: true
                preferredRendererType: VectorImage.CurveRenderer
            }
        }
    }
}
