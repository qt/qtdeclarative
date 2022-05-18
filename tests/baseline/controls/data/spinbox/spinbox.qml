import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ColumnLayout {
    spacing: 4
    width: 200

    SpinBox {
        value: 10
    }

    SpinBox {
        value: 10
        enabled: false
    }

    SpinBox {
        value: 10
        focus: true
    }

    SpinBox {
        value: 10
        LayoutMirroring.enabled: true
    }

    SpinBox {
        value: 10
        editable: true
    }

    SpinBox {
        value: 10
        up.hovered: true
    }

    SpinBox {
        value: 10
        up.pressed: true
    }

    SpinBox {
        value: 10
        down.hovered: true
    }

    SpinBox {
        value: 10
        down.pressed: true
    }
}
