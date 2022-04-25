import QtQuick
import QtQuick.Layouts

Item {
    RowLayout {
        Item {
            anchors.fill: parent
            x: 10
            y: 20
            width: 100
            height: 200
        }
    }

    Grid {
        Item {
            anchors.fill: parent
            x: 10
            y: 20
        }
    }

    Flow {
        Item { anchors.fill: parent;
            x: 10
            y: 20
        }
    }
}
