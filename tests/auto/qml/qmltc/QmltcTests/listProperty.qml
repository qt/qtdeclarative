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

    property QtObject appendMe: QtObject { id: a3; property string hello: "Hello from parent.children[2]" }

    ids: [a, a1, a2]

    property list<int> myList: [1,2,3,4]

    property int count: myList.length

    property int firstCount
    property int secondCount

    Component.onCompleted: {
        firstCount = count
        myList.push(43)
        secondCount = count
    }

}
