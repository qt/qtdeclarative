import QtQml

QtObject {
    id: root

    property var stash
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

    function run(op) {
        switch (op) {
        case "Length": return stash.length;
        case "IndexRead": return stash[0];
        case "IndexWrite": stash[0] = null; return true;
        case "ForIn": { var n = 0; for (var k in stash) ++n; return n; }
        case "Pop": stash.pop(); return true;
        case "Push": stash.push(null); return true;
        case "Shift": stash.shift(); return true;
        case "Unshift": stash.unshift(null); return true;
        case "Splice": stash.splice(0, 1); return true;
        case "IndexOf": return stash.indexOf(null);
        case "LastIndexOf": return stash.lastIndexOf(null);
        case "Sort": stash.sort(); return true;
        }
    }
}
