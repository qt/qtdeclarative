import QtQuick

QtObject {
    property string url
    property bool dataOK: false

    Component.onCompleted: {
        let xhr = new XMLHttpRequest;
        xhr.open("GET", url);
        xhr.overrideMimeType('text/xml');

        // Test to the end
        xhr.onreadystatechange = function() {
            if (xhr.readyState === XMLHttpRequest.DONE)
                dataOK = xhr.responseXML !== null;
        }

        xhr.send();
    }
}
