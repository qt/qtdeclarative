import QtQuick 2.15

Item {
    id: root
    property bool ok: false
    width: 640
    height: 480

    Text {
        id: text
        text: "This is a quite long text. Click me and i should remain visible!!! Sadly this doesn't happen"
        elide: Text.ElideRight
    }

    Component.onCompleted: {
        text.width = 300;
        text.height = 0;
        text.width = 0;
        text.height = 30;
        text.width = 300;
        root.ok = text.paintedWidth > 0 && text.paintedHeight > 0
    }
}
