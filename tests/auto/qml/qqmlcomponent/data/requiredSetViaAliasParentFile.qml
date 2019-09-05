import QtQuick 2.14

BaseWithRequired {
    id: withAlias
    property alias j: withAlias.i
    j: 42
}
