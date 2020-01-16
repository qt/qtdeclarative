import QtQuick 2.0

Item {
    property int rrr: 5
    Item {
        property int yyy: parent.rrr
    }
}
