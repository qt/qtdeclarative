import QtQml 2.15

QtObject {
    id: root
    objectName: "400"

    property Component c: Component {
        id: customItem
        QtObject {
            objectName: root.objectName
        }
    }

    property string result: {
        const properties = {
            "objectName": "42",
        }
        const item = customItem.createObject(root, properties)
        return item.objectName;
    }
}
