import Things

QtObject {
    default property QtObject child
    objectName: "QtQml was imported from Things/qmldir"

    ItemDerived {
        objectName: "QQuickItem is depended upon and we know its properties"
        x: 4
    }
}
