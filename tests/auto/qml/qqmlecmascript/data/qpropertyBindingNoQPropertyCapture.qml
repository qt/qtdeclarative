import QtQuick

Item {
    objectName: "redRectangle"
    id: redRectangle

    property bool b: false
    function toggle() { b = !b }
    width: b ? 600 : 500

    Item {
        id: blueRectangle
        objectName: "blueRectangle"
        // width: b ? (100 + redRectangle.width / 2) : 25
        width: b ? redRectangle.width : 25
    }

    property int blueRectangleWidth: blueRectangle.width
}
