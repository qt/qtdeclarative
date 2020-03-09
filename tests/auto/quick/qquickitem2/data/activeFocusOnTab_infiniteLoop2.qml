import QtQuick 2.14

Item {
    width: 400
    height: 200
    Item {
        objectName: "hiddenChild"
        focus: true
        activeFocusOnTab: true
        visible: false
    }
}
