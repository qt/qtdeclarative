import QtQml

QtObject {
    id: self

    function createList() : list<QtObject> { return [self] }
    Component.onCompleted: console.log(createList())
}
