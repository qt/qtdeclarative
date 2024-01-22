import QtQuick
import qmldirtest

Window {
    width: 640
    height: 480
    visible: true
    property color color: mybutton.color

    MyButton {
        id: mybutton
    }
}
