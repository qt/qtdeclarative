import QtQuick 2.13

Item {
    property alias i_alias: sub.i

    Item {
        id: sub
        required property int i
    }
}
