import QtQuick 2.0

Item {
    Image {
        transform: Rotation {
            Behavior on angle {
                SpringAnimation { spring: 2; damping: 0.2; modulus: 360 }
            }
            NumberAnimation on angle {
            }
        }
    }

    Image {
        transform: Rotation {
            NumberAnimation on angle {
            }
            Behavior on angle {
                SpringAnimation { spring: 2; damping: 0.2; modulus: 360 }
            }
        }
    }
}
