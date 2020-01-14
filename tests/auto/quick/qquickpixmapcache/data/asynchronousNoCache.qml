import QtQuick 2.12

Item {
    visible: true
    width: 640
    height: 480

    Image{
        asynchronous: true
        anchors.fill: parent
        fillMode: Image.Stretch
        source: "exists1.png"
        cache: false
        sourceSize.width: width/2
        sourceSize.height: height/2
    }
}
