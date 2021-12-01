import QtQml

QtObject {
    id: a

    property Component c: Component {
        QtObject {
            property int a: 5
            objectName: a.objectName
        }
    }
}
