import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ColumnLayout {
    width: 200
    height: 500
    spacing: 2

    RangeSlider {
        first.value: 0.25
        second.value: 0.75
        wheelEnabled: true
    }

    RangeSlider {
        first.value: 0.0
        second.value: 0.9
    }

    RangeSlider {
        first.value: 0.25
        second.value: 0.35
        width: 100
    }

    RangeSlider {
        first.value: 0.75
        second.value: 1.0
        snapMode: RangeSlider.SnapAlways
        height: 100
    }

    RangeSlider {
        from: 0.0
        stepSize: 0.2
        orientation: Qt.Vertical
        to: 1.0
        enabled: false
        first.value: 0.25
        second.value: 0.75
    }
}
