import QtQml

QtObject {
    id: a

    component Handle: QtObject {
        property int a: 5
        objectName: a.objectName
    }
}
