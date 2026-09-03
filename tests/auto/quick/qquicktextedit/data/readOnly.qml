import QtQuick

Rectangle {
    property variant myInput: input

    width: 800; height: 600; color: "blue"

    property bool activateSelectAllShortcut: false
    property bool activateCopyShortcut: false
    property bool activatePageupShortcut: false
    property bool acceptShortcutOverride: true

    Shortcut {
        sequences: [StandardKey.SelectAll]
        onActivated: { activateSelectAllShortcut = true }
    }

    Shortcut {
        sequences: [StandardKey.Copy]
        onActivated: { activateCopyShortcut = true }
    }

    Shortcut {
        sequences: [StandardKey.MoveToPreviousPage]
        onActivated: { activatePageupShortcut = true }
    }

    TextEdit {
        id: input

        focus: true
        readOnly: true
        text: "I am the very model of a modern major general.\n"
        Keys.onShortcutOverride: event => {
             event.accepted = acceptShortcutOverride
        }
    }
}
