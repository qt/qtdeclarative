import QtQuick 2.0

Rectangle {
    id: test
    property variant fontloader: fontloaderelement
    height: 100; width: 100
    property bool useotherfont: false
    property int statenum: 1
    property alias name: fontloaderelement.name
    property alias source: fontloaderelement.source
    property alias status: fontloaderelement.status

    FontLoader {
        id: fontloaderelement
    }

    states: [
        State { name: "start"; when: !useotherfont
            PropertyChanges { target: fontloaderelement; source: "tarzeau_ocr_a.ttf" }
        },
        State { name: "changefont"; when: useotherfont
            PropertyChanges { target: fontloaderelement; source: "daniel.ttf" }
        }
    ]

    Text { id: textelement; text: fontloaderelement.name; color: "black" }
}
