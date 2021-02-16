import QtQuick
Item {
    default property QtObject unused
    Text { } // must be added to the Item's default property, not `unused`
}
