import QtQuick 2.0

Image {
    transform: Rotation {
        angle: 6
        Behavior on angle {
            SpringAnimation { spring: 2; damping: 0.2; modulus: 360 }
        }
    }
}
