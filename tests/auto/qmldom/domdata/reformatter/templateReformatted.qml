import QtQuick 2.15

Text {
    property int i: 3

    text: "´" + "m
               l

              " + 'b' + '
              multi
              l
              l' + `template` + `t${i + 6}` + `t${i + `nested${i}`}` + `t${function () {
        return 5;
    }()}` + `t\${i}
              ${i + 2}`
    width: 300
}
