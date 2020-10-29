import QtQuick 2.0

QtObject {
    property string result
    property bool isString: false
    property bool isObject: false

    Component.onCompleted: {
        var a = Qt.resolvedUrl("resolvedUrl.qml");
        result = a;
        isString = (typeof a) == "string"
        isObject = (typeof a) == "object"
    }
}

