import QtQml 2.2
import example 1.0

QtObject {
    Component.onCompleted: {
        var opt1 = MyEnum.Option1A | MyEnum.Option1D // 0x09
        mydata.opt1 = opt1;
        mydata.setOpt1(opt1);
        mydata.setOption1(opt1);

        var opt2 = mydata.opt2;
        opt2 = (opt2 === MyEnum.Short8 ? MyEnum.Short16 : MyEnum.Short0);
        mydata.opt2 = opt2;
        mydata.setOpt2(opt2);
        mydata.setOption2(opt2);
    }
}
