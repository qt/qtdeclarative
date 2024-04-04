import QtQml

QtObject {
    id: self

    objectName: self.toString()

    Component.onCompleted: {
        self.destroy();
        self.destroy(25);
    }
}
