import QtQuick
import qmldirtest

Item {
    objectName: Name.name
    property color color: mybutton.color

    MyButton {
        id: mybutton
    }
}
