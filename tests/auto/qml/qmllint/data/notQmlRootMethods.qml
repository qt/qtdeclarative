import QtQml

QtObject {
    id: self

    function a() { self.deleteLater(); }
    function b() { self.destroyed(); }
}
