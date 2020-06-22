import Things

QtObject {
    objectName: "QtQml was imported from Things/qmldir"

    ItemDerived {
        objectName: "QQuickItem is depended upon and we know its properties"
        x: 4
    }
}
