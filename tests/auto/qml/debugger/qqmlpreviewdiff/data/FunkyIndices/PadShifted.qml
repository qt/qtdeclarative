import QtQuick

Item {
    Item {
        Text { text: "probe" }
    }
    property Component comp: Component {
        Widget { label: "dyn" }
    }
}
