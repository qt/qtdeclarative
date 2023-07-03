import QtQml

QtObject {
    id: root
    objectName: column.text

    property Component c: Component {
        id: comp
        QtObject { }
    }

    property QtObject rectItem: null

    property bool running: false

    property Timer t: Timer {
        id: column
        interval: 200
        running: root.running
        repeat: true

        property string text: {
            let item = root.rectItem
            let result = rectItem ? rectItem.objectName : "Create Object"
            return result
        }

        onTriggered: {
            let rectItem = root.rectItem

            // If rectItem exists destory it.
            if (rectItem) {
                rectItem.destroy()
                return
            }

            // Otherwise create a new object
            let newRectItem = comp.createObject(column, {})


            // Setting the objectName before setting root.rectItem seems to work.
            // newRectItem.width = 1200
            root.rectItem = newRectItem

            // But setting the objectName after setting root.rectItem seems to
            // cause the issue.
            newRectItem.objectName = "1300"
        }
    }
}
