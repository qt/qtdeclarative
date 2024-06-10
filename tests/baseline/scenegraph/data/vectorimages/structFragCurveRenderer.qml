import QtQuick
import QtQuick.VectorImage
import Qt.labs.folderlistmodel

Rectangle{
    id: topLevelItem
    width: 800
    height: 480

    Grid {
        columns: 3
        anchors.fill: parent
        Repeater {
            model: FolderListModel {
                folder: Qt.resolvedUrl("../shared/svg_12_testsuite/")
                nameFilters: [ "struct-frag*.svg"]
            }

            VectorImage {
                width: 266
                height: implicitHeight * width / implicitWidth
                source: fileUrl
                clip: true
                preferredRendererType: VectorImage.CurveRenderer
            }
        }
    }
}
