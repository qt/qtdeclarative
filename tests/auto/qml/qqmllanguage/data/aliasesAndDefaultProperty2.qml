import QtQuick

BasicTableView {
component  BasicTableView : Item {
    id: root
    default property alias __columns: root.data
    readonly property alias __listView: listView
    ListView {
        id: listView
    }
}

Item {}
}
