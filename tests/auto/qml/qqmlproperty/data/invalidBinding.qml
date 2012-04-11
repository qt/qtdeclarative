import QtQuick 2.0

Item {
    property Text text: myText

    property Rectangle rectangle1: myText
    property Rectangle rectangle2: eval('getMyText()') // eval to force non-shared (v8) binding

    function getMyText() { return myText; }

    Text {
        id: myText

        anchors.verticalCenter: parent // invalid binding
    }
}
