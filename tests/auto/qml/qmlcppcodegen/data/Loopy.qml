import QtQuick

Item {
    id: root

    property real radius: Math.floor(samples / 2);
    property int samples: 9
    property real deviation: (radius + 1.0) / 3.3333
    property int kernelRadius: Math.max(0, samples / 2);

    onSamplesChanged: rebuildStash();
    onDeviationChanged: rebuildStash();
    Component.onCompleted: rebuildStash();

    function rebuildStash() {
        var params = {
            radius: kernelRadius,
            deviation: Math.max(0.00001, deviation),
            fallback: root.radius != kernelRadius
        }

        stash = params
    }

    property var stash
}

