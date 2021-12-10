import QtQuick

Item {
  id: top;
  visible:true;
  width:300;
  height:300
  Text {
    id: text
    width: 30; height: 30
    text: "1234.56"
    font.bold: true
    Binding {
      target: text.font // which is not a QObject, so can't use it as a binding target
      property: 'bold'
      value: false;
      when: width < 30
    }
  }
}
