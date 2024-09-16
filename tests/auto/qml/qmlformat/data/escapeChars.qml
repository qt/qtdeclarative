import QtQuick

Item {
    x: {
        const s = "\""
        let a = {
            "\"": "\\"
        };

        let patron = {
            "\\\"\n\n" : "\?\?\\\"","": "", "\'\"\n":1
        };


    }
}
