import QtQuick 1.0
import Qt.test 1.0

Item {
    id: root

    AliasPropertyComponent {
        sourceComponent: returnsUndefined()
    }

    function returnsUndefined() {
        return undefined;
    }
}
