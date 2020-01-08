import QtQuick 2.0

QtObject {
    // Inputs

    id: root

    property string writeURL
    property string readURL
    // Outputs
    property bool writeDone: false
    property variant readResult

    Component.onCompleted: {
        // PUT
        var xhrWrite = new XMLHttpRequest;
        xhrWrite.open("PUT", writeURL);
        xhrWrite.onreadystatechange = function() {
            if (xhrWrite.readyState === XMLHttpRequest.DONE)
                writeDone = true;
        };
        xhrWrite.send("Test-String");
        // GET
        var xhrRead = new XMLHttpRequest;
        xhrRead.open("GET", readURL);
        xhrRead.onreadystatechange = function() {
            if (xhrRead.readyState === XMLHttpRequest.DONE)
                readResult = xhrRead.responseText;
        };
        xhrRead.send();
    }
}
