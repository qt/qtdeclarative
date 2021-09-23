import QtQuick 2.8

ListView {
    id: root
    width: 200
    height: 200

    delegate: Text {
        text: display
    }
}
