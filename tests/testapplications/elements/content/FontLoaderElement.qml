// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.0
import "font"

Item {
    id: fontloaderelementtest
    anchors.fill: parent
    property string testtext: ""
    property int currentfont: 0
    property variant availablefonts
    Component.onCompleted: { availablefonts = Qt.fontFamilies(); }

    Rectangle { anchors.fill: textitem; color: "lightsteelblue"; radius: 5 }
    FontLoader { id: fontloaderelement; name: availablefonts[currentfont] }

    Text {
        id: textitem
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 15
        height: 100; width: 250; text: fontloaderelement.name; font: fontloaderelement.name
        horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
    }

    SystemTestHelp { id: helpbubble; visible: statenum != 0
        anchors { top: parent.top; horizontalCenter: parent.horizontalCenter; topMargin: 50 }
    }
    BugPanel { id: bugpanel }

    states: [
        State { name: "start"; when: statenum == 1
            PropertyChanges { target: fontloaderelementtest
                testtext: "This is a Text item with the font defined via the FontLoader element.\n"+
                "Presently it should be indicating the font "+fontloaderelement.name+", displayed in said font.\n"+
                "Next, let's change the font to something else." }
        },
        State { name: "changefont"; when: statenum == 2
            PropertyChanges { target: fontloaderelementtest
                currentfont: 1;
                testtext: "FontLoader should now be displaying the text in "+fontloaderelement.name+".\n"+
                "Next let's repeatedly change the font." }
        },
        State { name: "switch"; when: statenum == 3
            PropertyChanges { target: fontticker; running: true }
            PropertyChanges { target: fontloaderelementtest
                testtext: "FontLoader should now be cycling between different fonts available on the system.\n"+
                "Next let's use a locally stored font." }
        },
        State { name: "uselocal"; when: statenum == 4
            PropertyChanges { target: fontticker; running: false }
            PropertyChanges { target: bugpanel; bugnumber: "20268" }
            PropertyChanges { target: fontloaderelement; source: "font/Vera.ttf"; name: "Bitstream Vera Sans"; }
            PropertyChanges { target: fontloaderelementtest
                testtext: "FontLoader should now be displaying the Bitstream Vera Sans font.\n"+
                "Advance to restart the test." }
        }
    ]
    Timer { id: fontticker; interval: 500; running: false; repeat: true; onTriggered: { currentfont = currentfont == availablefonts.length ? 0 : currentfont+1 } }
}
