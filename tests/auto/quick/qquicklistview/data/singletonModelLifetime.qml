import QtQuick 2.15
import QtQuick.Window 2.15
import test 1.0

Window {
    id: root
    visible: true
    width: 800
    height: 680
    property bool alive: false

    Component {
        id: view
        ListView {
            model: SingletonModel
        }
    }
    function compare(a,b) {
        root.alive = (a === b)
    }

    function test_singletonModelCrash() {
        SingletonModel.objectName = "model"
        var o = view.createObject(root)
        o.destroy()
        Qt.callLater(function() {
            compare(SingletonModel.objectName, "model")
        })
    }

    Component.onCompleted: root.test_singletonModelCrash()
}
