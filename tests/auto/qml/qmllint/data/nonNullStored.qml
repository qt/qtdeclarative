import QtQml

QtObject {
    property Foozle foozle
    function barzle() { return foozle.objectName }
}
