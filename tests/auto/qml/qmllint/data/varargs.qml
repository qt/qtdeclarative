import QtQml

Component {
    id: self
    onStatusChanged: createObject({a : 12}, self, 1, 2, 3)
}
