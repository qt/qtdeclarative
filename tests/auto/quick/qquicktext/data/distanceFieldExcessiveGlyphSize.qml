import QtQuick

Item {
    width: 200
    height: 100

    FontLoader {
        id: fontLoader
        objectName: "fontLoader"
        source: "testfont_glyphsize.ttf"
    }

    Text {
        objectName: "text"
        font: fontLoader.font
        renderType: Text.QtRendering
        text: "AB"
    }
}
