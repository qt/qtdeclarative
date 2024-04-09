import QtQml

QtObject {
    id: testObj

    // "readonly" means the identity of the list cannot be changed.
    // Its contents can be changed.
    readonly default property list<QtObject> theList

    Component.onCompleted: {
        for (var i = 0; i < 4; i++)
            testObj.theList.push(testObj)
    }

    property int l: theList.length
}

