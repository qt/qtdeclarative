import QtQml

QtObject {
    property string url: "testdocument.html"
    property bool dataOK: false

    Component.onCompleted: {
        var x = new XMLHttpRequest;
        x.onreadystatechange = function() {
            if (x.readyState === XMLHttpRequest.DONE) {
                x.onreadystatechange = function() {
                    dataOK = (x.responseText == "QML Rocks!\n");
                }
                x.open("GET", url);
                x.send();
            }
        }

        x.open("PUT", url);
        x.send("QML Rocks!\n")
    }
}
