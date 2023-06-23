import QtQml

QtObject {
    function burn() {
        return control.font
    }

    Component.onCompleted: {
        for (var a = 0; a < 10; ++a) {
            try { burn() } catch(e) {}
        }

        burn();
    }

}
