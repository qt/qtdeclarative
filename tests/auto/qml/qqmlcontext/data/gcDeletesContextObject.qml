import QtQml
QtObject {
    property Component c: MyItem {}
    property QtObject o: c.createObject()
}
