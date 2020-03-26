import QtQuick 2.15

import Test 1.0

Item {
    id: root

    property alias overlay: overlay

    Overlay {
        id: overlay
        // Parent it to the contentItem of the underlying QQuickWindow.
        parent: root.parent
    }
}
