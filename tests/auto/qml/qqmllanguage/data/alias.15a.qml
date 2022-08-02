import QtQuick 2.15

Item {
    id: root

    property alias symbol: symbol
    symbol.layer.enabled: true

    Item {
        id: symbol
    }
}
