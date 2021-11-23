import QtQuick
import QtQuick.Templates as T
import QtQuick.Controls.Basic

T.Dialog {
    id: control
    header: Label {
        background: Rectangle {
            width: parent.width + 1
            height: parent.height - 1
        }
    }

    property var n: 10
    property var a: {
        var x = 1;
        for (var i=0; i<n; ++i)
            x = x + x;
        return x;
    }
}
