import QtQuick
import QtQuick.Effects

Rectangle {
    width: 320
    height: 480
    color: "#c0c0c0"

    // Source item
    Rectangle {
        id: source1
        width: 60
        height: 60
        x: 20
        y: 20
        color: "#909000"
        border.width: 4
        border.color: "#f0f0f0"
        radius: 10
        visible: false
    }

    // Mask items
    Item {
        id: maskSource1
        width: source1.width
        height: source1.height
        layer.enabled: true
        visible: false
        Rectangle {
            anchors.fill: parent
            anchors.margins: 10
            radius: width / 2
            antialiasing: false
            smooth: true
            gradient: Gradient {
                GradientStop { position: 0.0; color: "#10ffffff" }
                GradientStop { position: 1.0; color: "#ffffffff" }
            }
        }
    }
    Rectangle {
        id: maskSource2
        width: source1.width
        height: source1.height
        layer.enabled: true
        visible: false
        gradient: Gradient {
            GradientStop { position: 0.0; color: "#10ffffff" }
            GradientStop { position: 1.0; color: "#ffffffff" }
        }
    }

    // Shadow
    MultiEffect {
        source: source1
        width: source1.width
        height: source1.height
        x: 20
        y: 20
        shadowEnabled: true
        shadowBlur: 0.0
        shadowHorizontalOffset: 5
        shadowVerticalOffset: 5
    }
    MultiEffect {
        source: source1
        width: source1.width
        height: source1.height
        x: 120
        y: 20
        shadowEnabled: true
        shadowBlur: 1.0
        shadowHorizontalOffset: 5
        shadowVerticalOffset: 5
    }
    MultiEffect {
        source: source1
        width: source1.width
        height: source1.height
        x: 220
        y: 20
        shadowEnabled: true
        shadowBlur: 1.0
        shadowHorizontalOffset: -5
        shadowVerticalOffset: -5
        shadowColor: "#802020"
    }

    // Mask
    MultiEffect {
        source: source1
        width: source1.width
        height: source1.height
        x: 20
        y: 120
        maskEnabled: true
        // No mask source
    }
    MultiEffect {
        source: source1
        width: source1.width
        height: source1.height
        x: 120
        y: 120
        maskEnabled: true
        maskSource: maskSource1
    }
    MultiEffect {
        source: source1
        width: source1.width
        height: source1.height
        x: 220
        y: 120
        maskEnabled: true
        maskSource: maskSource1
        maskInverted: true
    }

    // Mask 2
    MultiEffect {
        source: source1
        width: source1.width
        height: source1.height
        x: 20
        y: 220
        maskEnabled: true
        maskSource: maskSource1
        maskThresholdMax: 0.5
    }
    MultiEffect {
        source: source1
        width: source1.width
        height: source1.height
        x: 120
        y: 220
        maskEnabled: true
        maskSource: maskSource2
        maskThresholdMax: 0.5
        maskSpreadAtMax: 0.4
        // Switching mask
        Component.onCompleted: maskSource = maskSource1;
    }
    MultiEffect {
        source: source1
        width: source1.width
        height: source1.height
        x: 220
        y: 220
        maskEnabled: true
        maskSource: maskSource1
        maskThresholdMin: 0.4
        maskThresholdMax: 0.6
        maskSpreadAtMax: 0.4
        maskSpreadAtMin: 0.4
        maskInverted: true
    }

    // Multiple effects
    MultiEffect {
        source: source1
        width: source1.width
        height: source1.height
        x: 20
        y: 320
        maskEnabled: true
        maskSource: maskSource2
        maskThresholdMax: 0.5
        maskSpreadAtMax: 0.2
        shadowEnabled: true
        brightness: 0.5
    }
    MultiEffect {
        source: source1
        width: source1.width
        height: source1.height
        x: 120
        y: 320
        maskEnabled: true
        maskSource: maskSource2
        maskThresholdMax: 0.5
        maskSpreadAtMax: 0.2
        maskInverted: true
        shadowEnabled: true
        colorization: 1.0
        saturation: -0.5
        paddingRect.width: 10
        paddingRect.height: 10
    }
    MultiEffect {
        source: source1
        width: source1.width
        height: source1.height
        x: 220
        y: 320
        maskEnabled: true
        maskSource: maskSource2
        maskThresholdMin: 0.5
        maskSpreadAtMin: 0.2
        shadowEnabled: true
        shadowBlur: 0.6
        colorization: 1.0
        colorizationColor: "#00ff00"
        blurEnabled: true
        blur: 0.8
        paddingRect.width: 10
        paddingRect.height: 10
        rotation: 45
    }
}
