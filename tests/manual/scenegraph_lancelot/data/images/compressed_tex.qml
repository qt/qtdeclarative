import QtQuick 2.14

Rectangle {
    height: 480
    width: 320
    color: "green"

    Grid {
        anchors.fill: parent
        columns: 2
        topPadding: 40
        padding: 64
        spacing: 64
        rowSpacing: 48

        Image {
            source: "../shared/o1_bc1.ktx"
        }
        Image {
            source: "../shared/o1.png"
        }
        Image {
            source: "../shared/o2_bc1.ktx"
        }
        Image {
            source: "../shared/o2.png"
        }
        Image {
            source: "../shared/t1_bc2.ktx"
        }
        Image {
            source: "../shared/t1.png"
        }
        Image {
            source: "../shared/t2_bc2.ktx"
        }
        Image {
            source: "../shared/t2.png"
        }
    }
}
