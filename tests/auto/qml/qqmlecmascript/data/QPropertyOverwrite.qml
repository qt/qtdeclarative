import Qt.test 1

ClassWithQProperty {
    id: root
    value: 13

    property QPropertyBase prop: QPropertyBase {
        objectName: "test"
        value: root.value
    }
}
