import QtQuick
import TestTypes


Item {
    id: root

    property Moo485 moo

    readonly property Foo485 foo: Foo485 {
        uid: root.moo.uid ?? 0xFFFF
    }
}
