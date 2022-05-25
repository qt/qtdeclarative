import QtQml 2.0

QtObject {
    property int dummy: 42
    property int p: dummy
    property int watcher: 0

    onPChanged: {
        watcher = p; // NB: not a binding
    }
}
