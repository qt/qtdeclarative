import QtQuick
Item {
    Rectangle {
        TextEdit {}
    }
    property NumberAnimation p: SpringAnimation {}
    function foobar(one: Text, another) : Animation
    {
        return p
    }
    Item {}
}
