import QtQuick
import "."

Item {
    id: root

    component InlineType : Item {
        property int marker: 99
    }

    InlineType { id: instance }
}
