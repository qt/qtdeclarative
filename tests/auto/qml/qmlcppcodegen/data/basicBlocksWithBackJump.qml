pragma Strict
import QtQml

QtObject {
    function infinite() {
        var foo = false
        if (true) {
            while (true) {}
        } else {
            console.log(foo)
        }
    }

    function t1() {
        var i = 0
        var foo = false
        if (true) {
            for (i = 0; i < 42 ; ++i) {}
        } else {
            console.log(foo)
        }
    }

    function t2() {
        var foo = false
        if (false) {
            while(Math.random() < 0.5) {}
        } else {
            console.log(foo)
        }
    }

    function t3() {
        var foo = false
        if (Math.random() < 0.5) {
            console.log(foo)
        } else {
            while(Math.random() < 0.5) {}
            console.log(foo)
        }
    }
}
