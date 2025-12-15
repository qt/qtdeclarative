import QtQml 2.15
import qtbug127308

QtObject {
    Component.onCompleted: {
        console.log("Unscoped access:")
        try { console.log("EnumTester::Scoped",           EnumTester.S1,                  EnumTester.S2                  ); } catch (a) { console.log("EnumTester::Scoped",           a) }
        try { console.log("EnumTester::Unscoped",         EnumTester.U1,                  EnumTester.U2                  ); } catch (b) { console.log("EnumTester::Unscoped",         b) }
        try { console.log("EnumTesterScoped::Scoped",     EnumTesterScoped.S1,            EnumTesterScoped.S2            ); } catch (c) { console.log("EnumTesterScoped::Scoped",     c) }
        try { console.log("EnumTesterScoped::Unscoped",   EnumTesterScoped.U1,            EnumTesterScoped.U2            ); } catch (d) { console.log("EnumTesterScoped::Unscoped",   d) }
        try { console.log("EnumTesterUnscoped::Scoped",   EnumTesterUnscoped.S1,          EnumTesterUnscoped.S2          ); } catch (e) { console.log("EnumTesterUnscoped::Scoped",   e) }
        try { console.log("EnumTesterUnscoped::Unscoped", EnumTesterUnscoped.U1,          EnumTesterUnscoped.U2          ); } catch (f) { console.log("EnumTesterUnscoped::Unscoped", f) }
        console.log()
        console.log("Scoped access:")
        try { console.log("EnumTester::Scoped",           EnumTester.Scoped.S1,           EnumTester.Scoped.S2           ); } catch (g) { console.log("EnumTester::Scoped",           g) }
        try { console.log("EnumTester::Unscoped",         EnumTester.Unscoped.U1,         EnumTester.Unscoped.U2         ); } catch (h) { console.log("EnumTester::Unscoped",         h) }
        try { console.log("EnumTesterScoped::Scoped",     EnumTesterScoped.Scoped.S1,     EnumTesterScoped.Scoped.S2     ); } catch (i) { console.log("EnumTesterScoped::Scoped",     i) }
        try { console.log("EnumTesterScoped::Unscoped",   EnumTesterScoped.Unscoped.U1,   EnumTesterScoped.Unscoped.U2   ); } catch (j) { console.log("EnumTesterScoped::Unscoped",   j) }
        try { console.log("EnumTesterUnscoped::Scoped",   EnumTesterUnscoped.Scoped.S1,   EnumTesterUnscoped.Scoped.S2   ); } catch (k) { console.log("EnumTesterUnscoped::Scoped",   k) }
        try { console.log("EnumTesterUnscoped::Unscoped", EnumTesterUnscoped.Unscoped.U1, EnumTesterUnscoped.Unscoped.U2 ); } catch (l) { console.log("EnumTesterUnscoped::Unscoped", l) }
    }
}
