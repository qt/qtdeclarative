import QtQuick
import QtQuick.Controls

Rectangle
{
    objectName: "rectangle"
    width: 255
    height: 381
    visible: true

    Popup
    {
        objectName: "popup"
        anchors.centerIn: parent
        modal: true

        Rectangle
        {
            color: "red"
            width: 40
            height: 40
        }
    }
}
