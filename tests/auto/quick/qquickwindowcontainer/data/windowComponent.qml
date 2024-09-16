import QtQuick

Window {
    Item { id: itemParent; objectName: "itemParent" }
    Window { id: windowParent; objectName: "windowParent" }

    property QtObject window_item: null
    property QtObject window_window: null

    property QtObject window_item_parent: null
    property QtObject window_window_parent: null

    Component { id: windowComponent; Window{} }

    Component.onCompleted: {
        // Windows created with QObjects, Items, or Windows as their parent
        // do not their visual parent set via auto-parenting -- only their
        // transient parent.
        window_item = windowComponent.createObject(itemParent);
        window_window = windowComponent.createObject(windowParent);

        // A visual parent must be set explicitly
        window_item_parent = windowComponent.createObject(itemParent);
        window_item_parent.parent = itemParent;
        window_window_parent = windowComponent.createObject(windowParent);
        window_window_parent.parent = windowParent;
    }
}
