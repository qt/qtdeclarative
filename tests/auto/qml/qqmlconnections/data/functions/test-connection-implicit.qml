import QtQuick 2.0

Item {
    width: 50

    property bool tested: false

    Connections { function onWidthChanged() { tested = true } }
}
