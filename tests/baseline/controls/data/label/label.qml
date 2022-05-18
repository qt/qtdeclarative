import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ColumnLayout {
    width: 100

    Label {
        text: "Label"
    }

    Label {
        text: "Label"
        enabled: false
    }

    Label {
        text: "Label"
        LayoutMirroring.enabled: true
    }
}
