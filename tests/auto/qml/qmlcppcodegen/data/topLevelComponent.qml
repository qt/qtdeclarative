pragma Strict
import QtQml

Component {
    QtObject {
        id: root

        function myOpen() {
            root.objectName = "foo"
        }

        Component.onCompleted: myOpen()
    }
}
