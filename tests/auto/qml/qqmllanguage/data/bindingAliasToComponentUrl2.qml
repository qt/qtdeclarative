import QtQuick
Item {
    id: root
    Component {
        id: accessibleNormal
        ComponentType {
            id: inaccessibleNormal
        }
    }
    property alias accessibleNormalProgress: accessibleNormal.progress
}
