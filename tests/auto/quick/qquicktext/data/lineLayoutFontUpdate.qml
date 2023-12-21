import QtQuick

Item {
    width: 640
    height: 480

    FontLoader {
        id: fontIcons
        source: "tarzeau_ocr_a.ttf"
    }

    Text {
        id: exampleText
        objectName: "exampleText"
        text: "Example multiline text"
        wrapMode: Text.WordWrap
        width: 100
        onLineLaidOut: (line) => {
            if (line.number < 1) {
                line.x += 40;
                line.width -= 40;
            }
        }
    }
}
