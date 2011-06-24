/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/
#include <qtest.h>
#include <private/qdeclarativechangeset_p.h>

#define VERIFY_EXPECTED_OUTPUT



class tst_qdeclarativemodelchange : public QObject
{
    Q_OBJECT
public:
    struct Signal
    {
        int start;
        int end;
        int to;

        bool isInsert() const { return to == -1; }
        bool isRemove() const { return to == -2; }
        bool isMove() const { return to >= 0; }
        bool isChange() const { return to == -3; }
    };

    static Signal Insert(int start, int end) { Signal signal = { start, end, -1 }; return signal; }
    static Signal Remove(int start, int end) { Signal signal = { start, end, -2 }; return signal; }
    static Signal Move(int start, int end, int to) { Signal signal = { start, end, to }; return signal; }
    static Signal Change(int start, int end) { Signal signal = { start, end, -3 }; return signal; }

    typedef QVector<Signal> SignalList;


#ifdef VERIFY_EXPECTED_OUTPUT

    template<typename T>
    void move(int from, int to, int n, T *items)
    {
        if (from > to) {
            // Only move forwards - flip if backwards moving
            int tfrom = from;
            int tto = to;
            from = tto;
            to = tto+n;
            n = tfrom-tto;
        }

        T replaced;
        int i=0;
        typename T::ConstIterator it=items->begin(); it += from+n;
        for (; i<to-from; ++i,++it)
            replaced.append(*it);
        i=0;
        it=items->begin(); it += from;
        for (; i<n; ++i,++it)
            replaced.append(*it);
        typename T::ConstIterator f=replaced.begin();
        typename T::Iterator t=items->begin(); t += from;
        for (; f != replaced.end(); ++f, ++t)
            *t = *f;
    }

    QVector<int> applyChanges(const QVector<int> &list, const QVector<Signal> &changes)
    {
        QVector<int> alteredList = list;
        foreach (const Signal &signal, changes) {
            if (signal.isInsert()) {
                alteredList.insert(signal.start, signal.end - signal.start, 100);
            } else if (signal.isRemove()) {
                alteredList.erase(alteredList.begin() + signal.start, alteredList.begin() + signal.end);
            } else if (signal.isMove()) {
                move(signal.start, signal.to, signal.end - signal.start, &alteredList);
            } else if (signal.isChange()) {
                for (int i = signal.start; i < signal.end; ++i) {
                    if (alteredList[i] < 100)
                        alteredList[i] = 100;
                }
            }
        }
        return alteredList;
    }

#endif

private slots:
    void sequence_data();
    void sequence();
};

bool operator ==(const tst_qdeclarativemodelchange::Signal &left, const tst_qdeclarativemodelchange::Signal &right) {
    return left.start == right.start && left.end == right.end && left.to == right.to; }


QDebug operator <<(QDebug debug, const tst_qdeclarativemodelchange::Signal &signal)
{
    if (signal.isInsert())
        debug.nospace() << "Insert(" << signal.start << "," << signal.end << ")";
    else if (signal.isRemove())
        debug.nospace() << "Remove(" << signal.start << "," << signal.end << ")";
    else if (signal.isMove())
        debug.nospace() << "Move(" << signal.start << "," << signal.end << "," << signal.to << ")";
    else if (signal.isChange())
        debug.nospace() << "Change(" << signal.start << "," << signal.end << ")";
    return debug;
}

Q_DECLARE_METATYPE(tst_qdeclarativemodelchange::SignalList)

void tst_qdeclarativemodelchange::sequence_data()
{
    QTest::addColumn<SignalList>("input");
    QTest::addColumn<SignalList>("output");

    // Insert
    QTest::newRow("i(12-17)")
            << (SignalList() << Insert(12, 17))
            << (SignalList() << Insert(12, 17));
    QTest::newRow("i(2-5),i(12-17)")
            << (SignalList() << Insert(2, 5) << Insert(12, 17))
            << (SignalList() << Insert(2, 5) << Insert(12, 17));
    QTest::newRow("i(12-17),i(2-5)")
            << (SignalList() << Insert(12, 17) << Insert(2, 5))
            << (SignalList() << Insert(2, 5) << Insert(15, 20));
    QTest::newRow("i(12-17),i(12-15)")
            << (SignalList() << Insert(12, 17) << Insert(12, 15))
            << (SignalList() << Insert(12, 20));
    QTest::newRow("i(12-17),i(17-20)")
            << (SignalList() << Insert(12, 17) << Insert(17, 20))
            << (SignalList() << Insert(12, 20));
    QTest::newRow("i(12-17),i(15-18)")
            << (SignalList() << Insert(12, 17) << Insert(15, 18))
            << (SignalList() << Insert(12, 20));

    // Remove
    QTest::newRow("r(3-12)")
            << (SignalList() << Remove(3, 12))
            << (SignalList() << Remove(3, 12));
    QTest::newRow("r(3-7),r(3-5)")
            << (SignalList() << Remove(3, 7) << Remove(3, 5))
            << (SignalList() << Remove(3, 9));
    QTest::newRow("r(4-3),r(14-19)")
            << (SignalList() << Remove(4, 7) << Remove(14, 19))
            << (SignalList() << Remove(4, 7) << Remove(14, 19));
    QTest::newRow("r(14-19),r(4-7)")
            << (SignalList() << Remove(14, 19) << Remove(4, 7))
            << (SignalList() << Remove(4, 7) << Remove(11, 16));
    QTest::newRow("r(4-7),r(2-11)")
            << (SignalList() << Remove(4, 7) << Remove(2, 11))
            << (SignalList() << Remove(2, 14));

    // Move
    QTest::newRow("m(8-10,10)")
            << (SignalList() << Move(8, 10, 10))
            << (SignalList() << Move(8, 10, 10));
    // No merging of moves yet.
//    QTest::newRow("m(5-7,13),m(5-8,12)")
//            << (SignalList() << Move(5, 7, 13) << Move(5, 8, 12))
//            << (SignalList() << Move(5, 10, 10));

    // Change
    QTest::newRow("c(4-9)")
            << (SignalList() << Change(4, 9))
            << (SignalList() << Change(4, 9));
    QTest::newRow("c(4-9),c(12-14)")
            << (SignalList() << Change(4, 9) << Change(12, 14))
            << (SignalList() << Change(4, 9) << Change(12, 14));
    QTest::newRow("c(12-14),c(4-9)")
            << (SignalList() << Change(12, 14) << Change(4, 9))
            << (SignalList() << Change(4, 9) << Change(12, 14));
    QTest::newRow("c(4-9),c(2-4)")
            << (SignalList() << Change(4, 9) << Change(2, 4))
            << (SignalList() << Change(2, 9));
    QTest::newRow("c(4-9),c(9-11)")
            << (SignalList() << Change(4, 9) << Change(9, 11))
            << (SignalList() << Change(4, 11));
    QTest::newRow("c(4-9),c(3-5)")
            << (SignalList() << Change(4, 9) << Change(3, 5))
            << (SignalList() << Change(3, 9));
    QTest::newRow("c(4-9),c(8-10)")
            << (SignalList() << Change(4, 9) << Change(8, 10))
            << (SignalList() << Change(4, 10));
    QTest::newRow("c(4-9),c(3-5)")
            << (SignalList() << Change(4, 9) << Change(3, 5))
            << (SignalList() << Change(3, 9));
    QTest::newRow("c(4-9),c(2,11)")
            << (SignalList() << Change(4, 9) << Change(2, 11))
            << (SignalList() << Change(2, 11));
    QTest::newRow("c(4-9),c(12-15),c(8-14)")
            << (SignalList() << Change(4, 9) << Change(12, 15) << Change(8, 14))
            << (SignalList() << Change(4, 15));

    // Insert, then remove.
    QTest::newRow("i(12-18),r(12-18)")
            << (SignalList() << Insert(12, 18) << Remove(12, 18))
            << (SignalList());
    QTest::newRow("i(12-18),r(10-14)")
            << (SignalList() << Insert(12, 18) << Remove(10, 14))
            << (SignalList() << Remove(10, 12) << Insert(10, 14));
    QTest::newRow("i(12-18),r(16-20)")
            << (SignalList() << Insert(12, 18) << Remove(16, 20))
            << (SignalList() << Remove(12, 14) << Insert(12, 16));
    QTest::newRow("i(12-18),r(13-17)")
            << (SignalList() << Insert(12, 18) << Remove(13, 17))
            << (SignalList() << Insert(12, 14));
    QTest::newRow("i(12-18),r(14,18)")
            << (SignalList() << Insert(12, 18) << Remove(14, 18))
            << (SignalList() << Insert(12, 14));
    QTest::newRow("i(12-18),r(12-16)")
            << (SignalList() << Insert(12, 18) << Remove(12, 16))
            << (SignalList() << Insert(12, 14));
    QTest::newRow("i(12-18),r(11-19)")
            << (SignalList() << Insert(12, 18) << Remove(11, 19))
            << (SignalList() << Remove(11, 13));
    QTest::newRow("i(12-18),r(8-12)")
            << (SignalList() << Insert(12, 18) << Remove(8, 12))
            << (SignalList() << Remove(8, 12) << Insert(8, 14));
    QTest::newRow("i(12-18),r(2-6)")
            << (SignalList() << Insert(12, 18) << Remove(2, 6))
            << (SignalList() << Remove(2, 6) << Insert(8, 14));
    QTest::newRow("i(12-18),r(18-22)")
            << (SignalList() << Insert(12, 18) << Remove(18, 22))
            << (SignalList() << Remove(12, 16) << Insert(12, 18));
    QTest::newRow("i(12-18),r(20-24)")
            << (SignalList() << Insert(12, 18) << Remove(20, 24))
            << (SignalList() << Remove(14, 18) << Insert(12, 18));

    // Insert, then move
    QTest::newRow("i(12-18),m(12-18,5)")
            << (SignalList() << Insert(12, 18) << Move(12, 18, 5))
            << (SignalList() << Insert(5, 11));
    QTest::newRow("i(12-18),m(10-14,5)")
            << (SignalList() << Insert(12, 18) << Move(10, 14, 5))
            << (SignalList() << Insert(5, 7) << Insert(14, 18) << Move(12, 14, 5));
    QTest::newRow("i(12-18),m(16-20,5)")
            << (SignalList() << Insert(12, 18) << Move(16, 20, 5))
            << (SignalList() << Insert(5, 7) << Insert(14, 18) << Move(18, 20, 7));
    QTest::newRow("i(12-18),m(13-17,5)")
            << (SignalList() << Insert(12, 18) << Move(13, 17, 5))
            << (SignalList() << Insert(5, 9) << Insert(16, 18));
    QTest::newRow("i(12-18),m(14-18,5)")
            << (SignalList() << Insert(12, 18) << Move(14, 18, 5))
            << (SignalList() << Insert(5, 9) << Insert(16, 18));
    QTest::newRow("i(12-18),m(12-16,5)")
            << (SignalList() << Insert(12, 18) << Move(12, 16, 5))
            << (SignalList() << Insert(5, 9) << Insert(16, 18));
    QTest::newRow("i(12-18),m(11-19,5)")
            << (SignalList() << Insert(12, 18) << Move(11, 19, 5))
            << (SignalList() << Insert(5, 11) << Move(17, 18, 5) << Move(18, 19, 12));
    QTest::newRow("i(12-18),m(8-12,5)")
            << (SignalList() << Insert(12, 18) << Move(8, 12, 5))
            << (SignalList() << Insert(12, 18) << Move(8, 12, 5));
    QTest::newRow("i(12-18),m(2-6,5)")
            << (SignalList() << Insert(12, 18) << Move(2, 6, 5))
            << (SignalList() << Insert(12, 18) << Move(2, 6, 5));
    QTest::newRow("i(12-18),m(18-22,5)")
            << (SignalList() << Insert(12, 18) << Move(18, 22, 5))
            << (SignalList() << Insert(12, 18) << Move(18, 22, 5));
    QTest::newRow("i(12-18),m(20-24,5)")
            << (SignalList() << Insert(12, 18) << Move(20, 24, 5))
            << (SignalList() << Insert(12, 18) << Move(20, 24, 5));

    QTest::newRow("i(12-18),m(5-13,11)")
            << (SignalList() << Insert(12, 18) << Move(5, 13, 11))
            << (SignalList() << Insert(12, 17) << Insert(18, 19) << Move(5, 12, 11));

    QTest::newRow("i(12-18),m(12-18,23)")
            << (SignalList() << Insert(12, 18) << Move(12, 18, 23))
            << (SignalList() << Insert(23, 29));
    QTest::newRow("i(12-18),m(10-14,23)")
            << (SignalList() << Insert(12, 18) << Move(10, 14, 23))
            << (SignalList() << Insert(12, 16) << Insert(25, 27) << Move(10, 12, 23));
    QTest::newRow("i(12-18),m(16-20,23)")
            << (SignalList() << Insert(12, 18) << Move(16, 20, 23))
            << (SignalList() << Insert(12, 16) << Insert(25, 27) << Move(16, 18, 25));
    QTest::newRow("i(12-18),m(13-17,23)")
            << (SignalList() << Insert(12, 18) << Move(13, 17, 23))
            << (SignalList() << Insert(12, 14) << Insert(23, 27));
    QTest::newRow("i(12-18),m(14-18,23)")
            << (SignalList() << Insert(12, 18) << Move(14, 18, 23))
            << (SignalList() << Insert(12, 14) << Insert(23, 27));
    QTest::newRow("i(12-18),m(12-16,23)")
            << (SignalList() << Insert(12, 18) << Move(12, 16, 23))
            << (SignalList() << Insert(12, 14) << Insert(23, 27));
    QTest::newRow("i(12-18),m(11-19,23)")
            << (SignalList() << Insert(12, 18) << Move(11, 19, 23))
            << (SignalList() << Insert(25, 31) << Move(11, 12, 24) << Move(11, 12, 30));
    QTest::newRow("i(12-18),m(8-12,23)")
            << (SignalList() << Insert(12, 18) << Move(8, 12, 23))
            << (SignalList() << Insert(12, 18) << Move(8, 12, 23));
    QTest::newRow("i(12-18),m(2-6,23)")
            << (SignalList() << Insert(12, 18) << Move(2, 6, 23))
            << (SignalList() << Insert(12, 18) << Move(2, 6, 23));
    QTest::newRow("i(12-18),m(18-22,23)")
            << (SignalList() << Insert(12, 18) << Move(18, 22, 23))
            << (SignalList() << Insert(12, 18) << Move(18, 22, 23));
    QTest::newRow("i(12-18),m(20-24,23)")
            << (SignalList() << Insert(12, 18) << Move(20, 24, 23))
            << (SignalList() << Insert(12, 18) << Move(20, 24, 23));

    QTest::newRow("i(12-18),m(11-21,23)")
            << (SignalList() << Insert(12, 18) << Move(11, 21, 23))
            << (SignalList() << Insert(27, 33) << Move(11, 12, 26) << Move(11, 14, 30));

    // Insert, then change
    QTest::newRow("i(12-18),c(12-16)")
            << (SignalList() << Insert(12, 18) << Change(12, 6))
            << (SignalList() << Insert(12, 18));
    QTest::newRow("i(12-18),c(10-14)")
            << (SignalList() << Insert(12, 18) << Change(10, 16))
            << (SignalList() << Insert(12, 18) << Change(10, 12));
    QTest::newRow("i(12-18),c(16-20)")
            << (SignalList() << Insert(12, 18) << Change(16, 20))
            << (SignalList() << Insert(12, 18) << Change(18, 20));
    QTest::newRow("i(12-18),c(13-17)")
            << (SignalList() << Insert(12, 18) << Change(13, 17))
            << (SignalList() << Insert(12, 18));
    QTest::newRow("i(12-18),c(14-18)")
            << (SignalList() << Insert(12, 18) << Change(14, 18))
            << (SignalList() << Insert(12, 18));
    QTest::newRow("i(12-18),c(12-16)")
            << (SignalList() << Insert(12, 18) << Change(12, 16))
            << (SignalList() << Insert(12, 18));
    QTest::newRow("i(12-18),c(11-19)")
            << (SignalList() << Insert(12, 18) << Change(11, 19))
            << (SignalList() << Insert(12, 18) << Change(11, 12) << Change(18, 19));
    QTest::newRow("i(12-18),c(8-12)")
            << (SignalList() << Insert(12, 18) << Change(8, 12))
            << (SignalList() << Insert(12, 18) << Change(8, 12));
    QTest::newRow("i(12-18),c(2-6)")
            << (SignalList() << Insert(12, 18) << Change(2, 6))
            << (SignalList() << Insert(12, 18) << Change(2, 6));
    QTest::newRow("i(12-18),c(18-22)")
            << (SignalList() << Insert(12, 18) << Change(18, 22))
            << (SignalList() << Insert(12, 18) << Change(18, 22));
    QTest::newRow("i(12-18),c(20-24)")
            << (SignalList() << Insert(12, 18) << Change(20, 24))
            << (SignalList() << Insert(12, 18) << Change(20, 24));

    // Remove, then insert
    QTest::newRow("r(12-18),i(12-18)")
            << (SignalList() << Remove(12, 18) << Insert(12, 18))
            << (SignalList() << Remove(12, 18) << Insert(12, 18));
    QTest::newRow("r(12-18),i(10-14)")
            << (SignalList() << Remove(12, 18) << Insert(10, 14))
            << (SignalList() << Remove(12, 18) << Insert(10, 14));
    QTest::newRow("r(12-18),i(16-20)")
            << (SignalList() << Remove(12, 18) << Insert(16, 20))
            << (SignalList() << Remove(12, 18) << Insert(16, 20));
    QTest::newRow("r(12-18),i(13-17)")
            << (SignalList() << Remove(12, 18) << Insert(13, 17))
            << (SignalList() << Remove(12, 18) << Insert(13, 17));
    QTest::newRow("r(12-18),i(14-18)")
            << (SignalList() << Remove(12, 18) << Insert(14, 18))
            << (SignalList() << Remove(12, 18) << Insert(14, 18));
    QTest::newRow("r(12-18),i(12-16)")
            << (SignalList() << Remove(12, 18) << Insert(12, 16))
            << (SignalList() << Remove(12, 18) << Insert(12, 16));
    QTest::newRow("r(12-18),i(11-19)")
            << (SignalList() << Remove(12, 18) << Insert(11, 19))
            << (SignalList() << Remove(12, 18) << Insert(11, 19));
    QTest::newRow("i(12-18),r(8-12)")
            << (SignalList() << Remove(12, 18) << Insert(8, 12))
            << (SignalList() << Remove(12, 18) << Insert(8, 12));
    QTest::newRow("i(12-18),r(2-6)")
            << (SignalList() << Remove(12, 18) << Insert(2, 6))
            << (SignalList() << Remove(12, 18) << Insert(2, 6));
    QTest::newRow("i(12-18),r(18-22)")
            << (SignalList() << Remove(12, 18) << Insert(18, 22))
            << (SignalList() << Remove(12, 18) << Insert(18, 22));
    QTest::newRow("i(12-18),r(20-24)")
            << (SignalList() << Remove(12, 18) << Insert(20, 24))
            << (SignalList() << Remove(12, 18) << Insert(20, 24));

    // Move, then insert
    QTest::newRow("m(12-18,5),i(12-18)")
            << (SignalList() << Move(12, 18, 5) << Insert(12, 18))
            << (SignalList() << Insert(6, 12) << Move(18, 24, 5));
    QTest::newRow("m(12-18,5),i(10-14)")
            << (SignalList() << Move(12, 18, 5) << Insert(10, 14))
            << (SignalList() << Insert(5, 9) << Move(16, 21, 5) << Move(21, 22, 14));
    QTest::newRow("m(12-18,5),i(16-20)")
            << (SignalList() << Move(12, 18, 5) << Insert(16, 20))
            << (SignalList() << Insert(10, 14) << Move(16, 22, 5));
    QTest::newRow("m(12-18,5),i(13-17)")
            << (SignalList() << Move(12, 18, 5) << Insert(13, 17))
            << (SignalList() << Insert(7, 11) << Move(16, 22, 5));
    QTest::newRow("m(12-18,5),i(14-18)")
            << (SignalList() << Move(12, 18, 5) << Insert(14, 18))
            << (SignalList() << Insert(8, 12) << Move(16, 22, 5));
    QTest::newRow("m(12-18,5),i(12-16)")
            << (SignalList() << Move(12, 18, 5) << Insert(12, 16))
            << (SignalList() << Insert(6, 10) << Move(16, 22, 5));
    QTest::newRow("m(12-18,5),i(11-19)")
            << (SignalList() << Move(12, 18, 5) << Insert(11, 19))
            << (SignalList() << Insert(5, 13) << Move(20, 26, 5));
    QTest::newRow("m(12-18,5),i(8-12)")
            << (SignalList() << Move(12, 18, 5) << Insert(8, 12))
            << (SignalList() << Insert(5, 9) << Move(16, 19, 5) << Move(19, 22, 12));
    QTest::newRow("m(12-18,5),i(2-6)")
            << (SignalList() << Move(12, 18, 5) << Insert(2, 6))
            << (SignalList() << Insert(2, 6) << Move(16, 22, 9));
    QTest::newRow("m(12-18,5),i(18-22)")
            << (SignalList() << Move(12, 18, 5) << Insert(18, 22))
            << (SignalList() << Insert(18, 22) << Move(12, 18, 5));
    QTest::newRow("m(12-18,5),i(20-24)")
            << (SignalList() << Move(12, 18, 5) << Insert(20, 24))
            << (SignalList() << Insert(20, 24) << Move(12, 18, 5));

    QTest::newRow("m(12-18,23),i(12-18)")
            << (SignalList() << Move(12, 18, 23) << Insert(12, 18))
            << (SignalList() << Insert(12, 18) << Move(18, 24, 29));
    QTest::newRow("m(12-18,23),i(10-14)")
            << (SignalList() << Move(12, 18, 23) << Insert(10, 14))
            << (SignalList() << Insert(10, 14) << Move(16, 22, 27));
    QTest::newRow("m(12-18,23),i(16-20)")
            << (SignalList() << Move(12, 18, 23) << Insert(16, 20))
            << (SignalList() << Insert(22, 26) << Move(12, 18, 27));
    QTest::newRow("m(12-18,23),i(13-17)")
            << (SignalList() << Move(12, 18, 23) << Insert(13, 17))
            << (SignalList() << Insert(19, 23) << Move(12, 18, 27));
    QTest::newRow("m(12-18,23),i(14-18)")
            << (SignalList() << Move(12, 18, 23) << Insert(14, 18))
            << (SignalList() << Insert(20, 24) << Move(12, 18, 27));
    QTest::newRow("m(12-18,23),i(12-16)")
            << (SignalList() << Move(12, 18, 23) << Insert(12, 16))
            << (SignalList() << Insert(12, 16) << Move(16, 22, 27));
    QTest::newRow("m(12-18,23),i(11-19)")
            << (SignalList() << Move(12, 18, 23) << Insert(11, 19))
            << (SignalList() << Insert(11, 19) << Move(20, 26, 31));
    QTest::newRow("m(12-18,23),i(8-12)")
            << (SignalList() << Move(12, 18, 23) << Insert(8, 12))
            << (SignalList() << Insert(8, 12) << Move(16, 22, 27));
    QTest::newRow("m(12-18,23),i(2-6)")
            << (SignalList() << Move(12, 18, 23) << Insert(2, 6))
            << (SignalList() << Insert(2, 6) << Move(16, 22, 27));
    QTest::newRow("m(12-18,23),i(18-22)")
            << (SignalList() << Move(12, 18, 23) << Insert(18, 22))
            << (SignalList() << Insert(24, 28) << Move(12, 18, 27));
    QTest::newRow("m(12-18,23),i(20-24)")
            << (SignalList() << Move(12, 18, 23) << Insert(20, 24))
            << (SignalList() << Insert(26, 30) << Move(12, 18, 27));

    // Move, then remove
    QTest::newRow("m(12-18,5),r(12-18)")
            << (SignalList() << Move(12, 18, 5) << Remove(12, 18))
            << (SignalList() << Remove(6, 12) << Move(6, 12, 5));
    QTest::newRow("m(12-18,5),r(10-14)")
            << (SignalList() << Move(12, 18, 5) << Remove(10, 14))
            << (SignalList() << Remove(5, 8) << Remove(14, 15) << Move(9, 14, 5));
    QTest::newRow("m(12-18,5),r(16-20)")
            << (SignalList() << Move(12, 18, 5) << Remove(16, 20))
            << (SignalList() << Remove(10, 12) << Remove(16, 18) << Move(10, 16, 5));
    QTest::newRow("m(12-18,5),r(13-17)")
            << (SignalList() << Move(12, 18, 5) << Remove(13, 17))
            << (SignalList() << Remove(7, 11) << Move(8, 14, 5));
    QTest::newRow("m(12-18,5),r(14-18)")
            << (SignalList() << Move(12, 18, 5) << Remove(14, 18))
            << (SignalList() << Remove(8, 12) << Move(8, 14, 5));
    QTest::newRow("m(12-18,5),r(12-16)")
            << (SignalList() << Move(12, 18, 5) << Remove(12, 16))
            << (SignalList() << Remove(6, 10) << Move(8, 14, 5));
    QTest::newRow("m(12-18,5),r(11-19)")
            << (SignalList() << Move(12, 18, 5) << Remove(11, 19))
            << (SignalList() << Remove(5, 12) << Remove(11, 12));
    QTest::newRow("m(12-18,5),r(8-12)")
            << (SignalList() << Move(12, 18, 5) << Remove(8, 12))
            << (SignalList() << Remove(5, 6) << Remove(14, 17) << Move(11, 14, 5));
    QTest::newRow("m(12-18,5),r(2-6)")
            << (SignalList() << Move(12, 18, 5) << Remove(2, 6))
            << (SignalList() << Remove(2, 5) << Remove(9, 10) << Move(9, 14, 2));
    QTest::newRow("m(12-18,5),r(6-10)")
            << (SignalList() << Move(12, 18, 5) << Remove(6, 10))
            << (SignalList() << Remove(13, 17) << Move(12, 14, 5));
    QTest::newRow("m(12-18,5),r(18-22)")
            << (SignalList() << Move(12, 18, 5) << Remove(18, 22))
            << (SignalList() << Remove(18, 22) << Move(12, 18, 5));
    QTest::newRow("m(12-18,5),r(20-24)")
            << (SignalList() << Move(12, 18, 5) << Remove(20, 24))
            << (SignalList() << Remove(20, 24) << Move(12, 18, 5));

    QTest::newRow("m(12-18,23),r(12-18)")
            << (SignalList() << Move(12, 18, 23) << Remove(12, 18))
            << (SignalList() << Remove(18, 24) << Move(12, 18, 17));
    QTest::newRow("m(12-18,23),r(10-14)")
            << (SignalList() << Move(12, 18, 23) << Remove(10, 14))
            << (SignalList() << Remove(10, 12) << Remove(16, 18) << Move(10, 16, 19));
    QTest::newRow("m(12-18,23),r(16-20)")
            << (SignalList() << Move(12, 18, 23) << Remove(16, 20))
            << (SignalList() << Remove(22, 26) << Move(12, 18, 19));
    QTest::newRow("m(12-18,23),r(13-17)")
            << (SignalList() << Move(12, 18, 23) << Remove(13, 17))
            << (SignalList() << Remove(19, 23) << Move(12, 18, 19));
    QTest::newRow("m(12-18,23),r(14-18)")
            << (SignalList() << Move(12, 18, 23) << Remove(14, 18))
            << (SignalList() << Remove(20, 24) << Move(12, 18, 19));
    QTest::newRow("m(12-18,23),r(12-16)")
            << (SignalList() << Move(12, 18, 23) << Remove(12, 16))
            << (SignalList() << Remove(18, 22) << Move(12, 18, 19));
    QTest::newRow("m(12-18,23),r(11-19)")
            << (SignalList() << Move(12, 18, 23) << Remove(11, 19))
            << (SignalList() << Remove(11, 12) << Remove(17, 24) << Move(11, 17, 15));
    QTest::newRow("m(12-18,23),r(8-12)")
            << (SignalList() << Move(12, 18, 23) << Remove(8, 12))
            << (SignalList() << Remove(8, 12) << Move(8, 14, 19));
    QTest::newRow("m(12-18,23),r(2-6)")
            << (SignalList() << Move(12, 18, 23) << Remove(2, 6))
            << (SignalList() << Remove(2, 6) << Move(8, 14, 19));
    QTest::newRow("m(12-18,23),r(18-22)")
            << (SignalList() << Move(12, 18, 23) << Remove(18, 22))
            << (SignalList() << Remove(24, 28) << Move(12, 18, 19));
    QTest::newRow("m(12-18,23),r(20-24)")
            << (SignalList() << Move(12, 18, 23) << Remove(20, 24))
            << (SignalList() << Remove(12, 13) << Remove(25, 28) << Move(12, 17, 20));
}

void tst_qdeclarativemodelchange::sequence()
{
    QFETCH(SignalList, input);
    QFETCH(SignalList, output);

    QDeclarativeChangeSet set;

    foreach (const Signal &signal, input) {
        if (signal.isRemove())
            set.insertRemove(signal.start, signal.end);
        else if (signal.isInsert())
            set.insertInsert(signal.start, signal.end);
        else if (signal.isMove())
            set.insertMove(signal.start, signal.end, signal.to);
        else if (signal.isChange())
            set.insertChange(signal.start, signal.end);
    }

    SignalList changes;
    foreach (const QDeclarativeChangeSet::Remove &remove, set.removes())
        changes << Remove(remove.start, remove.end);
    foreach (const QDeclarativeChangeSet::Insert &insert, set.inserts())
        changes << Insert(insert.start, insert.end);
    foreach (const QDeclarativeChangeSet::Move &move, set.moves())
        changes << Move(move.start, move.end, move.to);
    foreach (const QDeclarativeChangeSet::Change &change, set.changes())
        changes << Change(change.start, change.end);

#ifdef VERIFY_EXPECTED_OUTPUT
    QVector<int> list;
    for (int i = 0; i < 40; ++i)
        list.append(i);
    QVector<int> inputList = applyChanges(list, input);
    QVector<int> outputList = applyChanges(list, output);
    if (outputList != inputList /* || changes != output*/) {
        qDebug() << input;
        qDebug() << output;
        qDebug() << changes;
        qDebug() << inputList;
        qDebug() << outputList;
    } else if (changes != output) {
        qDebug() << output;
        qDebug() << changes;
    }
    QCOMPARE(outputList, inputList);
#else

    if (changes != output) {
        qDebug() << output;
        qDebug() << changes;
    }

#endif

    QCOMPARE(changes, output);
}


QTEST_MAIN(tst_qdeclarativemodelchange)

#include "tst_qdeclarativechangeset.moc"
