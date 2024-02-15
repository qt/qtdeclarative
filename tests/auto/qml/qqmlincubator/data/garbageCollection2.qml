import QtQml
QtObject {
    property Component comp: Component {
        QtObject {}
    }

    property QtObject incubated: {
        var i = comp.incubateObject(null, {}, Qt.Synchronous);
        gc();
        return i.object
    }
}
