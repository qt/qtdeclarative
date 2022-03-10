import QtQuick

Rectangle {
    id: root
    property bool translucency: false

    gradient: Gradient {
        id: grad
    }

    onTranslucencyChanged: {
        if (translucency) {
            root.color = "transparent";
            root.gradient = null;
        } else {
            root.color = "white";
            root.gradient = grad;
        }
    }
}
