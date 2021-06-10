import QtQuick

Item {
  component A : B { required foo }
  component B : C { property QtObject foo }
  component C : Item {
  }
  A { foo: QtObject {} }
}

