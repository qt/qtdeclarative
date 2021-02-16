import QtQml 2.0

QtObject {
    property string hello: "Hello from parent"
    property list<QtObject> children

    children: [
        QtObject { property string hello: "Hello from parent.children[0]" },
        QtObject { property string hello: "Hello from parent.children[1]" }
    ]
}
