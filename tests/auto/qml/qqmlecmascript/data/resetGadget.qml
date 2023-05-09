import Qt.test

ResettableGadgetHolder {
    id: root
    property bool trigger: false
    onTriggerChanged: {
        root.g.value = undefined
    }
}
