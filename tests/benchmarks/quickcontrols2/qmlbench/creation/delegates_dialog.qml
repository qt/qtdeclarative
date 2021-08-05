import QtQuick
import QmlBench
import QtQuick.Controls

CreationBenchmark {
    id: root
    count: 20
    staticCount: 200
    delegate: Item {
        x: QmlBench.getRandom() * root.width - width
        y: QmlBench.getRandom() * root.height - height
        width: dialog.width
        height: dialog.height
        Dialog {
            id: dialog
            visible: true
            title: "Dialog"
            modal: (index % 100) === 0
            width: parent.width
            height: parent.height
            standardButtons: Dialog.Ok | Dialog.Cancel
            Item {
                implicitWidth: 200
                implicitHeight: 200
            }
        }
    }
}
