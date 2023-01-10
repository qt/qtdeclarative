import QtQuick

Text {
    property bool large: false
    property bool check: false
    font.pointSize: large ? 24 : 12
    font.letterSpacing: check ? 24 : 12

    Behavior on font.pointSize {
        SmoothedAnimation { duration: 100; }
    }

    Behavior on font.letterSpacing {
        SmoothedAnimation { duration: 100; }
    }

    Component.onCompleted: {
        large = true;
        large = false;
        check = true;
    }

    property real pointSize: font.pointSize
    property real letterSpacing: font.letterSpacing
}

