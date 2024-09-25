import QtQuick

Item {
    Item {
        x: Qt.uiLanguage
    }
    Item {
        EnterKey.type: Qt.EnterKeyGo
    }
    Item {
        x: (function(x) { return x + 1; })(42) + 2
    }
    Item {
        x: 42
    }

    Item {
        onXChanged: function() { let x = 32; return x + 1; }
    }
    Item {
        function hello() { console.log("hello"); }
        onXChanged: hello()
    }

    Item {
        id: asdf
        function hello() {
            if (y == 0)
                return;
            x = 1 + x + 3;
        }
    }

    Item {
        function writingX() { x = 32; return x + 1; }
    }

}
