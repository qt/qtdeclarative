pragma Strict
import QtQml
import QtQuick.Templates as T

T.SelectionRectangle {
    component Handle : QtObject {
        property QtObject rect: SelectionRectangle
    }
    property QtObject aa: Handle {}
}

