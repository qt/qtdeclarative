pragma ComponentBehavior: Bound

import QtQml

QtObject {
    id: root
    property string foo: "bar"
    property Component c1: Component {
        QtObject {
            id: cc
            objectName: root.foo
            property int i: 12
        }
    }

    property Component c2: Component {
        QtObject {
            objectName: root.foo + cc.i
        }
    }

    property QtObject o1: c1.createObject()
    property QtObject o2: c2.createObject()

}
