import QtQml

QtObject {
    id: window

    property var result;

    signal change(var data)

    onChange: data => {
        data["value"] = "Hello";
    }

    Component.onCompleted: {
        var mimeData = {};
        window.change(mimeData);
        result = mimeData;
    }
}