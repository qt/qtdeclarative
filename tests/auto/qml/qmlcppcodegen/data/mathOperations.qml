pragma Strict
import QtQml

QtObject {
    property double a
    property double b

    property int ia: a
    property int ib: b

    property bool ba: a
    property bool bb: b

    // unary double

    property double d_unot: !a
    property double d_uplus: +a
    property double d_uminus: -a
    property double d_ucompl: ~a
    property double d_increment: {
        var x = a;
        return ++x;
    }
    property double d_decrement: {
        var x = a;
        return --x;
    }

    property double d_bitAndConst: a & 9
    property double d_bitOrConst: a | 9
    property double d_bitXorConst: a ^ 9

    property double d_ushrConst: a >>> 9
    property double d_shrConst: a >> 9
    property double d_shlConst: a << 9

    // unary bool

    property double b_unot: !ba
    property double b_uplus: +ba
    property double b_uminus: -ba
    property double b_ucompl: ~ba
    property double b_increment: {
        var x = ba;
        return ++x;
    }
    property double b_decrement: {
        var x = ba;
        return --x;
    }

    property double b_bitAndConst: ba & 9
    property double b_bitOrConst: ba | 9
    property double b_bitXorConst: ba ^ 9

    property double b_ushrConst: ba >>> 9
    property double b_shrConst: ba >> 9
    property double b_shlConst: ba << 9

    // unary int

    property double i_unot: !ia
    property double i_uplus: +ia
    property double i_uminus: -ia
    property double i_ucompl: ~ia
    property double i_increment: {
        var x = ia;
        return ++x;
    }
    property double i_decrement: {
        var x = ia;
        return --x;
    }

    property double i_bitAndConst: ia & 9
    property double i_bitOrConst: ia | 9
    property double i_bitXorConst: ia ^ 9

    property double i_ushrConst: ia >>> 9
    property double i_shrConst: ia >> 9
    property double i_shlConst: ia << 9

    // double/double

    property double ddadd: a + b
    property double ddsub: a - b
    property double ddmul: a * b
    property double dddiv: a / b
    property double ddexp: a ** b
    property double ddmod: a % b

    property double ddbitAnd: a & b
    property double ddbitOr: a | b
    property double ddbitXor: a ^ b

    property double ddushr: a >>> b
    property double ddshr: a >> b
    property double ddshl: a << b

    // int/int

    property double iiadd: ia + ib
    property double iisub: ia - ib
    property double iimul: ia * ib
    property double iidiv: ia / ib
    property double iiexp: ia ** ib
    property double iimod: ia % ib

    property double iibitAnd: ia & ib
    property double iibitOr: ia | ib
    property double iibitXor: ia ^ ib

    property double iiushr: ia >>> ib
    property double iishr: ia >> ib
    property double iishl: ia << ib

    // bool/bool

    property double bbadd: ba + bb
    property double bbsub: ba - bb
    property double bbmul: ba * bb
    property double bbdiv: ba / bb
    property double bbexp: ba ** bb
    property double bbmod: ba % bb

    property double bbbitAnd: ba & bb
    property double bbbitOr: ba | bb
    property double bbbitXor: ba ^ bb

    property double bbushr: ba >>> bb
    property double bbshr: ba >> bb
    property double bbshl: ba << bb

    // int/double

    property double idadd: ia + b
    property double idsub: ia - b
    property double idmul: ia * b
    property double iddiv: ia / b
    property double idexp: ia ** b
    property double idmod: ia % b

    property double idbitAnd: ia & b
    property double idbitOr: ia | b
    property double idbitXor: ia ^ b

    property double idushr: ia >>> b
    property double idshr: ia >> b
    property double idshl: ia << b

    // double/int

    property double diadd: a + ib
    property double disub: a - ib
    property double dimul: a * ib
    property double didiv: a / ib
    property double diexp: a ** ib
    property double dimod: a % ib

    property double dibitAnd: a & ib
    property double dibitOr: a | ib
    property double dibitXor: a ^ ib

    property double diushr: a >>> ib
    property double dishr: a >> ib
    property double dishl: a << ib

    // bool/double

    property double bdadd: ba + b
    property double bdsub: ba - b
    property double bdmul: ba * b
    property double bddiv: ba / b
    property double bdexp: ba ** b
    property double bdmod: ba % b

    property double bdbitAnd: ba & b
    property double bdbitOr: ba | b
    property double bdbitXor: ba ^ b

    property double bdushr: ba >>> b
    property double bdshr: ba >> b
    property double bdshl: ba << b

    // double/bool

    property double dbadd: a + bb
    property double dbsub: a - bb
    property double dbmul: a * bb
    property double dbdiv: a / bb
    property double dbexp: a ** bb
    property double dbmod: a % bb

    property double dbbitAnd: a & bb
    property double dbbitOr: a | bb
    property double dbbitXor: a ^ bb

    property double dbushr: a >>> bb
    property double dbshr: a >> bb
    property double dbshl: a << bb

    // bool/int

    property double biadd: ba + ib
    property double bisub: ba - ib
    property double bimul: ba * ib
    property double bidiv: ba / ib
    property double biexp: ba ** ib
    property double bimod: ba % ib

    property double bibitAnd: ba & ib
    property double bibitOr: ba | ib
    property double bibitXor: ba ^ ib

    property double biushr: ba >>> ib
    property double bishr: ba >> ib
    property double bishl: ba << ib

    // int/bool

    property double ibadd: ia + bb
    property double ibsub: ia - bb
    property double ibmul: ia * bb
    property double ibdiv: ia / bb
    property double ibexp: ia ** bb
    property double ibmod: ia % bb

    property double ibbitAnd: ia & bb
    property double ibbitOr: ia | bb
    property double ibbitXor: ia ^ bb

    property double ibushr: ia >>> bb
    property double ibshr: ia >> bb
    property double ibshl: ia << bb
}
