import QtQml 2.0

QtObject {
    id: root
    readonly property QtObject test: QtObject { property int subproperty: 3}
    readonly property alias testAlias: root.test.subproperty
}

