import QtQml

QtObject {
    id: root
    property int status: -1
    Component.onCompleted: {
        let comp
        comp = Qt.createComponent("QtQml", "QtObject")
        if (comp.status !== Component.Ready) {
            root.status = 1
            return
        }

        comp = Qt.createComponent("QtQml", "QtObject", root)
        if (comp.status !== Component.Ready) {
            root.status = 2
            return
        }

        comp = Qt.createComponent("QtQml", "QtObject", Component.PreferSynchronous, root)
        if (comp.status !== Component.Ready) {
            root.status = 3
            return
        }


        comp = Qt.createComponent("QtQml", "QtObject", Component.Asynchronous)
        // C++ component will _always_ be ready, even if we request async loading
        if (comp.status !== Component.Ready) {
            root.status = 4
            return
        }

        comp = Qt.createComponent("QtQml", "DoesNotExist")
        if (comp.status !== Component.Error) {
            root.status = 5
            return
        }

        comp = Qt.createComponent("NoSuchModule", "QtObject")
        if (comp.status !== Component.Error) {
            root.status = 6
            return
        }

        root.status = 0
    }
}
