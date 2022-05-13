pragma Strict
pragma ComponentBehavior: Bound

import QtQml

QtObject {
    id: root
    property string foo: "bar"
    property Component c1: Component {
        QtObject {
            id: c1
            objectName: root.foo
            property int i: 12
            property Component c2: Component {
                QtObject {
                    objectName: root.foo + c1.i
                }
            }
            property QtObject o: c2.createObject()
        }
    }

    property QtObject o: c1.createObject()
}
