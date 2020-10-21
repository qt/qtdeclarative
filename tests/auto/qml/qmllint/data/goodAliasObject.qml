import QtQuick

Item {
    Item {
        QtObject { id: inner }

        property alias innerObj: inner
        property string name: innerObj.objectName
    }
}
