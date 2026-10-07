import QtQuick

Item {
    property Component comp: Component {
        Widget { label: "changed" }
    }
    Item {}
}
