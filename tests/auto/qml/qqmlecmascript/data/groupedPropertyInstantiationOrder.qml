import QtQuick 2.15

Item {

    width: 100
    height: 100

    component Screen01 : Rectangle {

        property Text myText: text1

        id: root

        width:  100
        height: 100

        Text {
            id: text1
            anchors.centerIn: parent
            color: "salmon"
            font.pixelSize: 14
        }
    }

    Screen01 {
        id: screen1
        property int speed : 42
        property int final_size: 14


        myText{
            font.pixelSize: 50 - final_size
            text: speed
        }
    }
}
