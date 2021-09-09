import QtQuick 2.0
import Other

QtObject {
    id: self
    property string result
    property bool isString: false
    property bool isObject: false

    Component.onCompleted: {
        var a = Qt.resolvedUrl("resolvedUrl.qml");
        result = a;
        isString = (typeof a) == "string"
        isObject = (typeof a) == "object"
    }

    property A here: A {
        there: self
        somewhere: "somewhere.qml"
    }

    property A there: A {
        somewhere: "somewhere.qml"
    }

    property url resolvedHere: here.here
    property url resolvedThere: there.here

    property var unresolvedUrl: Qt.url("nowhere/else.js")
}

