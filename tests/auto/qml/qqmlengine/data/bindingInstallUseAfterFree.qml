import QtQuick

Item {
    visible: false

    property int test: 1

    Component {
        id: comp
        Item {
            width: { width = test * 100 }
        }
    }

    Loader {
        sourceComponent: comp
        width: 100
    }
}
