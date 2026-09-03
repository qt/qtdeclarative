import QtQml

QtObject {
    property alias labelY: label.rect.y
    property alias labelWidth: label.rect.width

    labelY: { return 24 }
    labelWidth: 9

    property QtObject label: QtObject {
        id: label

        property rect rect
        rect.x: { return 12 }
    }
}
