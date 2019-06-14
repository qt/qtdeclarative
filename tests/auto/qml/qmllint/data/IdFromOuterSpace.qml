import QtQuick 2.0

Item {
    x: alien.x

    Component.onCompleted: {
        console.log(alien);
    }
}
