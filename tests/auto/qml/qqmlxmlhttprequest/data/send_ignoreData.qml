import QtQuick 2.0

QtObject {
    property string reqType
    property string url

    property bool dataOK: false

    Component.onCompleted: {
        var x = new XMLHttpRequest;
        x.open(reqType, url);

        // Test to the end
        x.onreadystatechange = function() {
            let expect = reqType == "HEAD" || reqType == "DELETE" ? "" : "QML Rocks!\n";
            if (x.readyState == XMLHttpRequest.DONE)
                dataOK = (x.responseText == expect);
        }

        x.send("Data To Ignore");
    }
}
