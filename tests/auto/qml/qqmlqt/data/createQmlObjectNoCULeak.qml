import QtQml

QtObject {
    id: root
    property int createCount: 0

    onCreateCountChanged: {
        let o = Qt.createQmlObject("import QtQml\nQtObject {  }\n", root);
        o.destroy();
    }
}
