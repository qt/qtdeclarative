import QtQuick 2.15

Item {
  id: root
  Loader {
    id: myloader
  }
  function f() {}
  Component.onCompleted: {
    myloader.setSource("Component.qml", {"i": root.f});
  }
}
