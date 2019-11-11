import QtQuick 2.0

Item {
    id: self

    QtObject {
        id: inner
    }

    property alias innerObj: inner
    property alias name: self.objectName
}
