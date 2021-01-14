export function withProp(root) {
  const component = Qt.createComponent("data/jsmodule/Dynamic.qml");
  const el = component.createObject(root, { value: 42 });
  return el.value;
}

