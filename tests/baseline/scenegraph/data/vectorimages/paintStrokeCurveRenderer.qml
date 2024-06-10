import QtQuick
import QtQuick.VectorImage
import Qt.labs.folderlistmodel

Rectangle{
    id: topLevelItem
    width: 800
    height: 500

    Grid {
        columns: 4
        anchors.fill: parent
        Repeater {
            model: FolderListModel {
                folder: Qt.resolvedUrl("../shared/svg_12_testsuite/")
                nameFilters: [ "paint-stroke*.svg"]
            }

            VectorImage {
                width: 200
                height: implicitHeight * width / implicitWidth
                source: fileUrl
                clip: true
                preferredRendererType: VectorImage.CurveRenderer
            }
        }
    }
}
