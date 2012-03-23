import QtQuick 2.0

QtObject {
    property QtObject prop1: null
    property QtObject prop2: QtObject {}

    property bool test1: prop1 ? true : false
    property bool test2: prop2 ? true : false

    property bool test3: prop1 == false
    property bool test4: prop1 === false

    property bool test5: prop2 == false
    property bool test6: prop2 === false
}

