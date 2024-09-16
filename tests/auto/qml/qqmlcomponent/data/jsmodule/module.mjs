export function withProp(root, prefix) {
  const component = Qt.createComponent(prefix + "data/jsmodule/Dynamic.qml");
  const el = component.createObject(root, { value: 42 });
  return el.value;
}

