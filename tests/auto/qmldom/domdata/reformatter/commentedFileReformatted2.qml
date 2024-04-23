// pre comment
import QtQuick 2.15

// pre item comment
/* multi
line */ // comment after multi line
Item {
    function method(x) // function comment
    { // just returns the double of x
        return 2 * x; // yep twice as much
    } // post method

    // binding comment
    a: {
        // header

        // before x()
        // before x()
        x(); // after x
        // before y = 8 + z + zz

        // before y = 8 + z + zz
        y = 8 +
        // before z
        z + // after z
        // before zz
        zz - // after z + zz
        /*before (a b)*/(/*  before a  */ a * /* after a  */ b * /*after b*/ c); // after (a * b * c)

        if (y == 6) // if comment
            console.log("pippo");

        a + b; // comment

        // footer
    }
    // post binding comment
}
// footer file comment
/* second comment */ /* third comment */
