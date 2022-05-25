import QtQuick

Item {
    Item {
        property string what: "child"
        Item {
            property string what: "child.child"
        }
    }
}
