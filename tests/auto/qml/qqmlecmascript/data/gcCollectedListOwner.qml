pragma ComponentBehavior: Bound
import QtQml

QtObject {
    id: root

    property bool flip: false
    property QtObject a: QtObject {}
    property QtObject b: QtObject {}

    property Component component: Component {
        QtObject {
            property QtObject holder: QtObject {
                property list<QtObject> items: root.flip ? [root.a] : [root.b]
            }
        }
    }

    function createAndForget() { component.createObject(null) }
}
