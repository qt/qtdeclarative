import QtQml

QtObject {
    id: root

    // Holds the QmlListWrapper for holder's "items" list<> property.
    property var stash

    // The object that actually owns the list<> property.
    property QtObject holder

    property Component comp: Component {
        QtObject {
            property list<QtObject> items: [ QtObject {}, QtObject {} ]
        }
    }

    function grab() {
        holder = comp.createObject(root);
        stash = holder.items;
    }

    function drop() {
        holder.destroy();
    }
}
