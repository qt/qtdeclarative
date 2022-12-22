import QtQuick
import QtQuick.Effects

Rectangle {
    width: 320
    height: 480
    color: "#c0c0c0"

    // Source items
    Rectangle {
        id: source1
        width: 60
        height: 60
        color: "#909000"
        border.width: 4
        border.color: "#f0f0f0"
        radius: 30
        visible: false
    }
    Image {
        id: source2
        width: 60
        height: 60
        source: "../shared/t1.png"
        visible: false
    }
    Rectangle {
        id: source3
        width: 64
        height: 64
        color: "#909000"
        border.width: 4
        border.color: "#f0f0f0"
        visible: false
        layer.enabled: true
    }

    // Sources as-is
    MultiEffect {
        source: source1
        width: source1.width
        height: source1.height
        x: 20
        y: 20
    }
    MultiEffect {
        source: source2
        width: source2.width
        height: source2.height
        x: 120
        y: 20
    }
    MultiEffect {
        source: source3
        width: source3.width
        height: source3.height
        x: 220
        y: 20
    }

    // Sources changing
    MultiEffect {
        source: source1
        width: source1.width
        height: source1.height
        x: 20
        y: 120
        colorization: 1.0
        shadowEnabled: true
        shadowBlur: 0.5
        shadowHorizontalOffset: 5
        Component.onCompleted: source = source2;
    }
    MultiEffect {
        source: source2
        width: source2.width
        height: source2.height
        x: 120
        y: 120
        colorization: 1.0
        blurEnabled: true
        blur: 1.0
        Component.onCompleted: source = source1;
    }
    MultiEffect {
        source: source3
        width: source3.width
        height: source3.height
        x: 220
        y: 120
        colorization: 1.0
        colorizationColor: "#0000ff"
        blurEnabled: true
        blur: 1.0
        Component.onCompleted: source = source2;
        // Note: Without this the source3 sourceRect will
        // be adjusted which changes also other users.
        // This is due to how SourceProxy uses layer.enabled items directly.
        autoPaddingEnabled: false
    }

    // Padding
    MultiEffect {
        source: source1
        width: source1.width
        height: source1.height
        x: 20
        y: 220
        contrast: -0.5
        shadowEnabled: true
        shadowBlur: 1.0
        shadowScale: 1.2
        autoPaddingEnabled: false
    }
    MultiEffect {
        source: source1
        width: source1.width
        height: source1.height
        x: 120
        y: 220
        contrast: -0.5
        shadowEnabled: true
        shadowBlur: 1.0
        shadowVerticalOffset: 10
        shadowHorizontalOffset: 10
        // With autopadding, smaller addition needed for shadow
        paddingRect: Qt.rect(0, 0, 10, 10)
        autoPaddingEnabled: true
    }
    MultiEffect {
        source: source1
        width: source1.width
        height: source1.height
        x: 220
        y: 220
        contrast: -0.5
        shadowEnabled: true
        shadowBlur: 1.0
        shadowVerticalOffset: 10
        shadowHorizontalOffset: 10
        // Without autopadding, bigger addition needed for shadow
        paddingRect: Qt.rect(30, 30, 40, 40)
        autoPaddingEnabled: false
    }

    // Using layer.effect
    Image {
        width: 60
        height: 60
        x: 20
        y: 320
        source: "../shared/t1.png"
        layer.enabled: true
        layer.effect: MultiEffect {
            colorization: 1.0
            colorizationColor: "#ff00ff"
        }
    }
    Rectangle {
        id: rect2
        width: 60
        height: 60
        x: 120
        y: 320
        color: "#909000"
        border.width: 4
        border.color: "#f0f0f0"
        radius: 30
        layer.enabled: true
        layer.effect: MultiEffect {
            saturation: 1.0
            shadowEnabled: true
            shadowVerticalOffset: 5
            shadowHorizontalOffset: 5
        }
    }
    Rectangle {
        width: 60
        height: 60
        x: 220
        y: 320
        color: "#909000"
        border.width: 4
        border.color: "#f0f0f0"
        radius: 30
        layer.enabled: true
        layer.effect: multiEffect1
        Component {
            id: multiEffect1
            MultiEffect {
                saturation: 1.0
                shadowEnabled: true
                shadowVerticalOffset: 5
                shadowHorizontalOffset: 5
            }
        }
    }
}
