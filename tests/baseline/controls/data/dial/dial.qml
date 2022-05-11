import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ColumnLayout {
    width: 400
    height: 800
    spacing: 30

    Dial {
        from: 0
        to: 1
        value: 0.1
    }

    Dial {
        from: 0.6
        to: 1
        value: 0.1
    }

    Dial {
        from: 0
        to: 0.5
        value: 0.1
    }

    Dial {
        from: 0.8
        to: 0.9
        value: 0.1
        enabled: false
    }
}
