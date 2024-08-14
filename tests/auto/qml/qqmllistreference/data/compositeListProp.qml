import QtQml

QtObject {
    id: self
    property list<AListItem> items: [ self ]

    Component.onCompleted: {
        items.push(self);
        items.push(null);
        items[2] = self;
        items.splice(1, 0, self);
        items.unshift(self);
    }
}
