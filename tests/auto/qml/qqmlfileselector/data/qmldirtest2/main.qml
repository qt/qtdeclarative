import QtQuick

Item {
    objectName: Name.name
    property color color: mybutton.color
    MyButton {
        id: mybutton
    }
}
