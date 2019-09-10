import QtQuick 2.14

Item {
    id: withAlias
    j: 42
    required property int i
    property alias j: withAlias.i
}
