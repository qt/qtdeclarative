import QtQuick 2.0
import QtQuick as QQ

Zzz {
    id: root
    width: height
    Rectangle {
        color: "green"
        anchors.fill: parent
        width: root.height
        height: root.foo.height

    }

    function lala() {}
    property Rectangle foo: Rectangle{ height: 200 }

    QQ.Rectangle {
        color:"red"
    }
}
