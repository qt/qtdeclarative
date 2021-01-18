import QtQuick 2.15

Item {
  id: root
  property alias bar: root.foo
  property var foo: function() { return false }

  Component.onCompleted: root.bar = function() { return true }
}
