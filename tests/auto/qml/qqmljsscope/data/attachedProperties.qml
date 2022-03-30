import QtQuick

AttachedBase {
    Keys.enabled: true
    Keys.forwardTo: [ foo ]

    Item {
        id: foo
    }
}
