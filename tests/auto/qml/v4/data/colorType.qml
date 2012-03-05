import QtQuick 2.0

QtObject {
    property bool useMyColor: true
    property color myColor: "red"
    property color myOtherColor: "green"

    property color test1: useMyColor ? myColor : myOtherColor
    property color test2: useMyColor ? "red" : "green"
    property color test3: useMyColor ? myColor : "green"

    property bool test4: !myColor ? false : true

    property bool test5: myColor != "red"
    property bool test6: myColor == "#ff0000"
    property bool test7: myColor != "#00ff00"
}

