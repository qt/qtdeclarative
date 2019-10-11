import QtQuick 2.12

Item {
    id: root
    width: 600
    height: 300

    TextInput {
        id: qwe
        objectName: "qwe"
        width: 500
        height: 100
        font.pixelSize: 50
        text: "123456"
        focus: true
    }

    Component.onCompleted: {
        qwe.insert(0, "***")
        qwe.remove(0, 3)
    }
}
