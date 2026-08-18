pragma ComponentBehavior: Bound
import QtQml

QtObject {
    id: root

    property bool flip: false
    property QtObject a: QtObject {}
    property QtObject b: QtObject {}

    property Component component: Component {
        QtObject {
            property Connections connections: Connections {
                target: root.flip ? root.a : root.b
                function onObjectNameChanged() {}
            }
        }
    }

    function createAndForget() { component.createObject(null) }
}
