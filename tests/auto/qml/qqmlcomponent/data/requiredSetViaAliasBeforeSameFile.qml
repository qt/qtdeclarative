import QtQuick 2.14

Item {
    id: withAlias
    required property int i
    j: 42
    property alias j: withAlias.i
}
