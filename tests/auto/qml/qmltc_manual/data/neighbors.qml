import QtQuick

Item {
    id: root
    QtObject {
        id: child1
        property int p: 41
        property int p2: child2.count * 2
    }

    LocallyImported {
        id: child2
        property int p: child1.p + 1
    }
}
