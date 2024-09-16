import QtQuick

Item {
    function data() { return 42; }

    function f(x) {
        let letStatement = data();
        const constStatement = data();
        var varStatement = data(); // bad?
    }

    function objects(x) {
        let { deconstructed } = data();
        let { deconstructedAloneWithInitializer = 55 } = data();
        let { deconstructedWithInitializer1 = 55, deconstructedWithInitializer2 = 66 } = data(), { unused: deconstructedWithInitializer3 = 77, } = data() ;
    }

    function arrays(x) {
        let [ deconstructed ] = data();
        let [ deconstructedAloneWithInitializer = 55 ] = data();
        let [ deconstructedWithInitializer1 = 55, deconstructedWithInitializer2 = 66 ] = data(), [ deconstructedWithInitializer3 = 77, ] = data() ;
    }

    function oneArrayingToRuleThemAll(x) {
        let [ head, [headOfSecond = 44, secondOfSecond = 55], [_ = "useless", secondOfThird = g()], [ [ headOfHeadOfFourth = g() + 1] ] ] = data();
    }

    function needleInTheHarraystack(x) {
        let [ head, [headOfSecond = 44, secondOfSecond = 55], [_ = "useless", secondOfThird = g(), { needle = "x" }], [ [ headOfHeadOfFourth = g() + 1] ] ] = data();
    }

    function arrayInTheObject(x) {
        let { p: [first, second] } = { p: [1,2] };
    }

}
