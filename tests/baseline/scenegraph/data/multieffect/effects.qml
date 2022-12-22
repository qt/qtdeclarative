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
    }

    // No changes
    MultiEffect {
        source: source1
        width: source1.width
        height: source1.height
        x: 120
        y: 20
    }

    // No changes
    MultiEffect {
        source: source1
        width: source1.width
        height: source1.height
        x: 220
        y: 20
        brightness: 0.0
        colorization: 0.0
        contrast: 0.0
        saturation: 0.0
        blurEnabled: true
    }

    // Color effects
    MultiEffect {
        source: source1
        width: source1.width
        height: source1.height
        x: 20
        y: 120
        brightness: 0.5
    }
    MultiEffect {
        source: source1
        width: source1.width
        height: source1.height
        x: 120
        y: 120
        contrast: -0.5
    }
    MultiEffect {
        source: source1
        width: source1.width
        height: source1.height
        x: 220
        y: 120
        saturation: -0.8
    }

    // Color effects 2
    MultiEffect {
        source: source1
        width: source1.width
        height: source1.height
        x: 20
        y: 220
        colorization: 0.5
        colorizationColor: "#ff0000"
    }
    MultiEffect {
        source: source1
        width: source1.width
        height: source1.height
        x: 120
        y: 220
        colorization: 0.5
        colorizationColor: "#ff0000"
        contrast: 0.5
        brightness: 0.5
    }
    MultiEffect {
        source: source1
        width: source1.width
        height: source1.height
        x: 220
        y: 220
        colorization: 1.0
        colorizationColor: "#ff0000"
        // saturation overrides colorization
        saturation: -1.0
    }

    // Blur
    MultiEffect {
        source: source1
        width: source1.width
        height: source1.height
        x: 20
        y: 320
        blurEnabled: true
        blur: 0.5
    }
    MultiEffect {
        source: source1
        width: source1.width
        height: source1.height
        x: 120
        y: 320
        blurEnabled: true
        blur: 1.0
    }
    MultiEffect {
        source: source1
        width: source1.width
        height: source1.height
        x: 220
        y: 320
        blurEnabled: true
        blur: 1.0
        blurMax: 50
    }
}
