import QtQuick 2.0

Item {
    width: 320
    height: 480

    Text {
        id: t1
        x: 10
        y: 10
        width: parent.width - 20
        wrapMode: Text.Wrap
        font.family: "Arial"
        font.pixelSize: 16
        textFormat: Text.RichText
        text: "<table border=1>"
              + "<tr><td>Column 1</td><td>Column 2</td><td>Column 3</td></tr>"
              + "<tr><td>Arma virumque cano, Troiae qui primus ab oris</td></tr>"
              + "</table>"
    }

    Text {
        id: t2
        x: 10
        y: t1.y + t1.height + 10
        width: parent.width - 20
        wrapMode: Text.Wrap
        font.family: "Arial"
        font.pixelSize: 16
        textFormat: Text.RichText
        text: "<table border=1>"
              + "<tr><td>Column 1</td><td>Column 2</td><td>Column 3</td></tr>"
              + "<tr><td></td><td colspan=2>Italiam</td></tr>"
              + "</table>"
    }

    Text {
        id: t3
        x: 10
        y: t2.y + t2.height + 10
        width: parent.width - 20
        wrapMode: Text.Wrap
        font.family: "Arial"
        font.pixelSize: 16
        textFormat: Text.RichText
        text: "<table border=1>"
              + "<tr><td>Column 1</td><td>Column 2</td><td>Column 3</td></tr>"
              + "<tr><td></td><td colspan=2>fato profugus, Laviniaque venit litora, multum ille et</td></tr>"
              + "</table>"
    }

    Text {
        id: t4
        x: 10
        y: t3.y + t3.height + 10
        width: parent.width - 20
        wrapMode: Text.Wrap
        font.family: "Arial"
        font.pixelSize: 16
        textFormat: Text.RichText
        text: "<table border=1>"
              + "<tr><td>Column 1</td><td>Column 2</td><td>Column 3</td></tr>"
              + "<tr><td colspan=3>terris</td></tr>"
              + "</table>"
    }

}
