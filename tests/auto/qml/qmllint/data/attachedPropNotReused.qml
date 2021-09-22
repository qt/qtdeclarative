import QtQuick

Text {
    text: KeyNavigation.priority == KeyNavigation.BeforeItem ? "before" : "after"
    Text {
        text: KeyNavigation.priority == KeyNavigation.BeforeItem ? "before" : "after"
    }
}
