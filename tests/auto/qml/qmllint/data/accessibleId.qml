import QtQml

QtObject {
    id: a
    property Component c: Component {
        QtObject {
            id: a
            property QtObject o: QtObject {
                property int a: 5
                objectName: a.objectName
            }
        }
    }
}
