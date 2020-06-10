import QtQuick 2.15
import QtQml.Models 2.15
import Qt.treemodel

PathView {
    width: 320
    height: 240
    function setRoot(index) {
        vdm.rootIndex = vdm.modelIndex(index);
    }
    model: DelegateModel {
        id: vdm
        model: TreeModelCpp
        delegate: Text {
            required property string display
            objectName: "wrapper"
            text: display
        }
    }

    path: Path {
        startX: 0; startY: 120
        PathLine { x: 320; y: 120 }
    }
}
