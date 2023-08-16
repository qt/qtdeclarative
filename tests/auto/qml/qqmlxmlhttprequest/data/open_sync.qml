import QtQuick 2.0

QtObject {
    property url url
    property string responseText

    Component.onCompleted: {
        var x = new XMLHttpRequest;
        x.open("GET", url, false);
        x.send();
        responseText = x.responseText;
    }
}

