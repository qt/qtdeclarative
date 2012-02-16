import QtQuick 2.0

Item {
    QtObject {
        id: object
        property bool prop1: true
        function myfunction() { return true; }
        property bool prop2: object.prop1 && myfunction();
    }

    property bool test1: object.prop1 && object.prop2
    property bool test2: object.prop1

    Component.onCompleted: {
        object.prop1 = false;
    }
}

