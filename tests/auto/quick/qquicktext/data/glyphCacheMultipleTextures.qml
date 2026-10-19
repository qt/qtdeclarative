import QtQuick

Rectangle {
    id: root
    width: 400
    height: 200
    color: "white"

    property int stage: 1

    readonly property int columns: 16
    readonly property int firstStageGlyphs: 32
    readonly property int totalGlyphs: 94 // Printable ASCII, except space

    function glyphs(first, count) {
        let text = ""
        for (let i = first; i < first + count; ++i) {
            if (i > first && i % columns === 0)
                text += "\n"
            text += String.fromCharCode(33 + i)
        }
        return text
    }

    FontLoader {
        id: fontLoader
        source: "tarzeau_ocr_a.ttf"
    }

    Column {
        Text {
            objectName: "firstStage"
            text: root.glyphs(0, root.firstStageGlyphs)
            font.family: fontLoader.font.family
            font.pixelSize: 20
            lineHeightMode: Text.FixedHeight
            lineHeight: 24
            renderType: Text.QtRendering
            renderTypeQuality: 192
        }

        Text {
            objectName: "secondStage"
            text: root.stage >= 2 ? root.glyphs(root.firstStageGlyphs, root.totalGlyphs - root.firstStageGlyphs) : ""
            font.family: fontLoader.font.family
            font.pixelSize: 20
            lineHeightMode: Text.FixedHeight
            lineHeight: 24
            renderType: Text.QtRendering
            renderTypeQuality: 192
        }
    }
}
