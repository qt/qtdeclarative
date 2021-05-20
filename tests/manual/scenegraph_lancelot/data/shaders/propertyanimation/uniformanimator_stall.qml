import QtQuick 2.12

// This test is a rotating rectangle with a uniform animator changing its color.
// There is a timer interrupting the rotation, but the uniform animator will still
// run. This is repeated a few times and then all animation is stopped.

Item {
    width: 320
    height: 320
    visible: true

    function stall(milliseconds) {
        var startTime = new Date().getTime();
        while ((new Date().getTime()) - startTime < milliseconds) {}
    }

    ShaderEffect {
        id: shader
        x: 60
        y: 60
        width: 200
        height: 200
        property real colorProperty: 0.0
        fragmentShader: "qrc:shaders/property.frag.qsb"
        UniformAnimator {
            id: animator
            target: shader
            uniform: "colorProperty"
            duration: 950
            from: 0.0
            to: 1.0
            loops: 10
            running: true
        }
        NumberAnimation on rotation { from: 0;to: 360; duration: 2500; loops: 1 }
    }

    Timer {
        interval: 600; running: true; repeat: true;
        property int num_repeats: 0
        onTriggered: {
            if (num_repeats < 3) {
                stall(550);
            } else {
                animator.running = false;
                repeat = false;
            }

            num_repeats += 1;
        }
    }
}
