pragma Strict

import QtQuick
import duck.tick
import duck.track

Item {
    property int x1: tick.x
    property int x2: track.x

    Tick {
        id: tick
    }

    Track {
        id: track
    }
}
