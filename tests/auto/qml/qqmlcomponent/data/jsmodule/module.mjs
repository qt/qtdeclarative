export function withProp(root) {
  const prefix = Qt.platform.os == "android" ? "qrc:" : "";
  const component = Qt.createComponent(prefix + "data/jsmodule/Dynamic.qml");
  const el = component.createObject(root, { value: 42 });
  return el.value;
}

