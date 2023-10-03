import QtQuick
Item {
    id: root
    Component {
        id: accessibleNormal
        Item {}
    }
    property alias accessibleNormalUrl: accessibleNormal.url
    property url urlClone: root.accessibleNormalUrl // crashes qml utility
}
