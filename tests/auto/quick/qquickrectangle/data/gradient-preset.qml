import QtQuick 2.0

Item {
    Rectangle {
        objectName: "enum"
        gradient: Gradient.NightFade
    }
    Rectangle {
        objectName: "string"
        gradient: "NightFade"
    }
    Rectangle {
        objectName: "invalid1"
        gradient: -1
    }
    Rectangle {
        objectName: "invalid2"
        gradient: 123456789
    }
    Rectangle {
        objectName: "invalid3"
        gradient: "NOT_EXISTING"
    }
    Rectangle {
        objectName: "invalid4"
        gradient: "NumPresets"
    }
    Rectangle {
        objectName: "invalid5"
        gradient: Gradient.NumPresets
    }
}
