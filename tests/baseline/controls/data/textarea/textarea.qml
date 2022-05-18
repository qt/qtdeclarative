import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ColumnLayout {
    spacing: 2
    width: 200

    TextArea {
        text: "TextArea\n...\n...\n...\n..."
    }

    TextArea {
        placeholderText: "TextArea\n...\n...\n..."
        enabled: false
    }

    TextArea {
        placeholderText: "TextArea\n...\n...\n..."
        focus: true
        cursorDelegate: Item {}
    }

    TextArea {
        text: "TextArea\n...\n...\n...\n..."
        LayoutMirroring.enabled: true
    }
}


