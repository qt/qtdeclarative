import QtQuick 2.0

Item {
    Item {
        id: target
        property int test: 42
    }
    Component.onCompleted: {
        with(target) {
            console.log(test);
        }
    }
}
