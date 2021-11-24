import QtQuick

Text {
    id: root
    text: KeyNavigation.priority == KeyNavigation.BeforeItem ? "before" : "after"
    Text {
        text: root.KeyNavigation.priority == KeyNavigation.BeforeItem ? "before" : "after"
    }
}
