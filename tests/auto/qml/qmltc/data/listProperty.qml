import QtQml 2.0

QtObject {
    id: a
    property string hello: "Hello from parent"
    property list<QtObject> children
    property list<QtObject> ids

    children: [
        QtObject { id: a1; property string hello: "Hello from parent.children[0]" },
        QtObject { id: a2; property string hello: "Hello from parent.children[1]" }
    ]

    ids: [a, a1, a2]
}
