import QtQuick 2.0

Item {
    property bool test: false

    property var items: [1, 2, 3, "four", "five"]
    property int bound: items[0]

    Component.onCompleted: {
        if (bound != 1) return false;

        items = [10, 2, 3, "four", "five"]  // bound should now be 10

        if (bound != 10) return false;

        test = true;
    }
}
