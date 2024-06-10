import QtQuick
import QtQuick.VectorImage
import Qt.labs.folderlistmodel

Rectangle{
    id: topLevelItem
    width: 800
    height: 300

    Grid {
        columns: 2
        anchors.fill: parent
        Repeater {
            model: ListModel {
                ListElement {
                    fileUrl: "../shared/svg_12_testsuite/text-tselect-03-t.svg"
                }
                ListElement {
                    fileUrl: "../shared/svg_12_testsuite/text-edit-201-t.svg"
                }
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
