import QtQuick
import QtQuick.VectorImage
import Qt.labs.folderlistmodel

Rectangle{
    id: topLevelItem
    width: 800
    height: 650

    Grid {
        columns: 2
        anchors.fill: parent
        Repeater {
            model: ListModel {
                ListElement {
                    fileUrl: "../shared/svg_12_testsuite/coords-constr-201-t.svg"
                }
                ListElement {
                    fileUrl: "../shared/svg_12_testsuite/coords-coord-01-t.svg"
                }
                ListElement {
                    fileUrl: "../shared/svg_12_testsuite/coords-units-01-t.svg"
                }
                ListElement {
                    fileUrl: "../shared/svg_12_testsuite/coords-viewattr-05-t.svg"
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
