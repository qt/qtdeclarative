import QtQml 2.0

QtObject {
    id: self

    property QtObject someObject
    default property alias child: self.someObject
}
