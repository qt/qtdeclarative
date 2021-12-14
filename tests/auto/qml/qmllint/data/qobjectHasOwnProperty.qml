import QtQml

QtObject {
    id: root

    property bool hasFooProperty: root.hasOwnProperty('foo')
}
