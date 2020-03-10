import QtQuick 2.12

Item {
    id: item
    property bool success: false
    property bool sanityCheck: false
    property int __underscore_property: 0
    on__Underscore_propertyChanged: item.sanityCheck = true

    Connections {
        target: item
        on__Underscore_propertyChanged: item.success = true
    }
}
