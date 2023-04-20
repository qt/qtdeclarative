import QtQuick 2.0

QtObject {
    property string url
    property string expectedURL

    property bool dataOK: false

    Component.onCompleted: {
        var x = new XMLHttpRequest;
        x.open("GET", url);

        // Test to the end
        x.onreadystatechange = function() {
            if (x.readyState === XMLHttpRequest.DONE)
                dataOK = (x.responseURL === expectedURL);
        }

        x.send()
    }
}
