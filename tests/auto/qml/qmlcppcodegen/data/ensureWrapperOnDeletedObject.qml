import QtQuick

Item {
    id: root

    property int rounds: 0

    Item { id: target }

    // The rectangles are QObject-children of the holder, but visual children of target.
    // Destroying the holder flags all of them as deleted before the first one is deleted.
    // Each deletion changes target.children while the others are still around.
    Component {
        id: holderComponent
        Item {
            id: holder
            required property Item display
            Rectangle { parent: holder.display; width: 10; height: 10 }
            Rectangle { parent: holder.display; width: 10; height: 10 }
        }
    }

    // Compiled by qmlcachegen. childAt() is a QObject-returning method. The compiled code
    // wraps the result, which is null for an object that is flagged as deleted.
    // The dependency on target.children has to be used, or qmlcachegen drops it as dead code
    // and the binding is never re-evaluated.
    property bool hasItem: {
        if (target.children.length === 0)
            return false;
        return target.childAt(1, 1) !== null;
    }

    Timer {
        interval: 1
        running: true
        repeat: true
        onTriggered: {
            const holder = holderComponent.createObject(root, { display: target });
            holder.destroy();

            // Allocate, so that an incremental gc cycle starts. It returns to the event loop
            // before it finishes, which is when the deferred delete is delivered.
            let garbage = [];
            for (let i = 0; i < 1000; ++i)
                garbage.push({ index: i });

            root.rounds += 1;
        }
    }
}
