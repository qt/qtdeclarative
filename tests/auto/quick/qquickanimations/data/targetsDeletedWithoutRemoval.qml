import QtQuick

Item {
    id: root

    property list<Translate> targets
    property alias animTargets: animation.targets

    Component {
        id: trComponent
        Translate {}
    }

    Component.onCompleted: {
        const target = trComponent.createObject(this);
        targets.push(target);
        target.destroy();
        // give event loop some time to actually stop the animation and destroy the target
        Qt.callLater(animation.start);
    }

    NumberAnimation {
        id: animation
        targets: root.targets
        property: "x"
        running: false
        to: 100
    }
}
