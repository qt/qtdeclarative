/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
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
        int index;
        int count;
        int to;
        int moveId;

        bool isInsert() const { return to == -1; }
        bool isRemove() const { return to == -2; }
        bool isMove() const { return to >= 0; }
        bool isChange() const { return to == -3; }
    };

    static Signal Insert(int index, int count, int moveId = -1) { Signal signal = { index, count, -1, moveId }; return signal; }
    static Signal Remove(int index, int count, int moveId = -1) { Signal signal = { index, count, -2, moveId }; return signal; }
    static Signal Move(int from, int to, int count) { Signal signal = { from, count, to, -1 }; return signal; }
    static Signal Change(int index, int count) { Signal signal = { index, count, -3, -1 }; return signal; }

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
        QHash<int, QVector<int> > removedValues;
        QVector<int> alteredList = list;
        foreach (const Signal &signal, changes) {
            if (signal.isInsert()) {
                if (signal.moveId != -1) {
                    QVector<int> tail = alteredList.mid(signal.index);
                    alteredList = alteredList.mid(0, signal.index) + removedValues.take(signal.moveId) + tail;
                } else {
                    alteredList.insert(signal.index, signal.count, 100);
                }
            } else if (signal.isRemove()) {
                if (signal.moveId != -1)
                    removedValues.insert(signal.moveId, alteredList.mid(signal.index, signal.count));
                alteredList.erase(alteredList.begin() + signal.index, alteredList.begin() + signal.index + signal.count);
            } else if (signal.isMove()) {
                move(signal.index, signal.to, signal.count, &alteredList);
            } else if (signal.isChange()) {
                for (int i = signal.index; i < signal.index + signal.count; ++i) {
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

bool operator ==(const tst_qdeclarativemodelchange::Signal &left, const tst_qdeclarativemodelchange::Signal &right)
{
    return left.index == right.index
            && left.count == right.count
            && left.to == right.to
            && ((left.moveId == -1 && right.moveId == -1) || (left.moveId != -1 && right.moveId != -1));
}


QDebug operator <<(QDebug debug, const tst_qdeclarativemodelchange::Signal &signal)
{
    if (signal.isInsert())
        debug.nospace() << "Insert(" << signal.index << "," << signal.count << "," << signal.moveId << ")";
    else if (signal.isRemove())
        debug.nospace() << "Remove(" << signal.index << "," << signal.count << "," << signal.moveId << ")";
    else if (signal.isMove())
        debug.nospace() << "Move(" << signal.index << "," << signal.to << "," << signal.count << ")";
    else if (signal.isChange())
        debug.nospace() << "Change(" << signal.index << "," << signal.count << ")";
    return debug;
}

Q_DECLARE_METATYPE(tst_qdeclarativemodelchange::SignalList)

void tst_qdeclarativemodelchange::sequence_data()
{
    QTest::addColumn<SignalList>("input");
    QTest::addColumn<SignalList>("output");

    // Insert
    QTest::newRow("i(12,5)")
            << (SignalList() << Insert(12,5))
            << (SignalList() << Insert(12,5));
    QTest::newRow("i(2,3),i(12,5)")
            << (SignalList() << Insert(2,3) << Insert(12,5))
            << (SignalList() << Insert(2,3) << Insert(12,5));
    QTest::newRow("i(12,5),i(2,3)")
            << (SignalList() << Insert(12,5) << Insert(2,3))
            << (SignalList() << Insert(2,3) << Insert(15,5));
    QTest::newRow("i(12,5),i(12,3)")
            << (SignalList() << Insert(12,5) << Insert(12,3))
            << (SignalList() << Insert(12,8));
    QTest::newRow("i(12,5),i(17,3)")
            << (SignalList() << Insert(12,5) << Insert(17,3))
            << (SignalList() << Insert(12,8));
    QTest::newRow("i(12,5),i(15,3)")
            << (SignalList() << Insert(12,5) << Insert(15,3))
            << (SignalList() << Insert(12,8));

    // Remove
    QTest::newRow("r(3,9)")
            << (SignalList() << Remove(3,9))
            << (SignalList() << Remove(3,9));
    QTest::newRow("r(3,4),r(3,2)")
            << (SignalList() << Remove(3,4) << Remove(3,2))
            << (SignalList() << Remove(3,6));
    QTest::newRow("r(4,3),r(14,5)")
            << (SignalList() << Remove(4,3) << Remove(14,5))
            << (SignalList() << Remove(4,3) << Remove(14,5));
    QTest::newRow("r(14,5),r(4,3)")
            << (SignalList() << Remove(14,5) << Remove(4,3))
            << (SignalList() << Remove(4,3) << Remove(11,5));
    QTest::newRow("r(4,3),r(2,9)")
            << (SignalList() << Remove(4,3) << Remove(2,9))
            << (SignalList() << Remove(2,12));

    // Move
    QTest::newRow("m(8-10,2)")
            << (SignalList() << Move(8,10,2))
            << (SignalList() << Remove(8,2,1) << Insert(10,2,1));

    QTest::newRow("m(23-12,6),m(13-15,5)")
            << (SignalList() << Move(23,12,6) << Move(13,15,5))
            << (SignalList() << Remove(23,1,0) << Remove(23,5,1) << Insert(12,1,0) << Insert(15,5,1));
    QTest::newRow("m(23-12,6),m(13-15,2)")
            << (SignalList() << Move(23,12,6) << Move(13,20,2))
            << (SignalList() << Remove(23,1,0) << Remove(23,2,1) << Remove(23,3,2) << Insert(12,1,0) << Insert(13,3,2) << Insert(20,2,1));
    QTest::newRow("m(23-12,6),m(13-2,2)")
            << (SignalList() << Move(23,12,6) << Move(13,2,2))
            << (SignalList() << Remove(23,1,0) << Remove(23,2,1) << Remove(23,3,2) << Insert(2,2,1) << Insert(14,1,0) << Insert(15,3,2));
    QTest::newRow("m(23-12,6),m(12-6,5)")
            << (SignalList() << Move(23,12,6) << Move(12,6,5))
            << (SignalList() << Remove(23,5,0) << Remove(23,1,1) << Insert(6,5,0) << Insert(17,1,1));
    QTest::newRow("m(23-12,6),m(10-5,4)")
            << (SignalList() << Move(23,12,6) << Move(10,5,4))
            << (SignalList() << Remove(10,2,0) << Remove(21,2,1) << Remove(21,4,2) << Insert(5,2,0) << Insert(7,2,1) << Insert(14,4,2));
    QTest::newRow("m(23-12,6),m(16-5,4)")
            << (SignalList() << Move(23,12,6) << Move(16,5,4))
            << (SignalList() << Remove(12,2,0) << Remove(21,4,1) << Remove(21,2,2) << Insert(5,2,2) << Insert(7,2,0) << Insert(16,4,1));
    QTest::newRow("m(23-12,6),m(13-5,4)")
            << (SignalList() << Move(23,12,6) << Move(13,5,4))
            << (SignalList() << Remove(23,1,0) << Remove(23,4,1) << Remove(23,1,2) << Insert(5,4,1) << Insert(16,1,0) << Insert(17,1,2));
    QTest::newRow("m(23-12,6),m(14-5,4)")
            << (SignalList() << Move(23,12,6) << Move(14,5,4))
            << (SignalList() << Remove(23,2,0) << Remove(23,4,1) << Insert(5,4,1) << Insert(16,2,0));
    QTest::newRow("m(23-12,6),m(12-5,4)")
            << (SignalList() << Move(23,12,6) << Move(12,5,4))
            << (SignalList() << Remove(23,4,0) << Remove(23,2,1) << Insert(5,4,0) << Insert(16,2,1));
    QTest::newRow("m(23-12,6),m(11-5,8)")
            << (SignalList() << Move(23,12,6) << Move(11,5,8))
            << (SignalList() << Remove(11,1,0) << Remove(11,1,1) << Remove(21,6,2) << Insert(5,1,0) << Insert(6,6,2) << Insert(12,1,1));
    QTest::newRow("m(23-12,6),m(8-5,4)")
            << (SignalList() << Move(23,12,6) << Move(8,5,4))
            << (SignalList() << Remove(8,4,0) << Remove(19,6,1) << Insert(5,4,0) << Insert(12,6,1));
    QTest::newRow("m(23-12,6),m(2-5,4)")
            << (SignalList() << Move(23,12,6) << Move(2,5,4))
            << (SignalList() << Remove(2,4,0) << Remove(19,6,1) << Insert(5,4,0) << Insert(12,6,1));
    QTest::newRow("m(23-12,6),m(18-5,4)")
            << (SignalList() << Move(23,12,6) << Move(18,5,4))
            << (SignalList() << Remove(12,4,0) << Remove(19,6,1) << Insert(5,4,0) << Insert(16,6,1));
    QTest::newRow("m(23-12,6),m(20-5,4)")
            << (SignalList() << Move(23,12,6) << Move(20,5,4))
            << (SignalList() << Remove(14,4,0) << Remove(19,6,1) << Insert(5,4,0) << Insert(16,6,1));

    QTest::newRow("m(23-12,6),m(5-13,11)")
            << (SignalList() << Move(23,12,6) << Move(5,13,11))
            << (SignalList() << Remove(5,7,0) << Remove(16,4,1) << Remove(16,2,2) << Insert(5,2,2) << Insert(13,7,0) << Insert(20,4,1));

    QTest::newRow("m(23-12,6),m(12-23,6)")
            << (SignalList() << Move(23,12,6) << Move(12,23,6))
            << (SignalList() << Remove(23,6,0) << Insert(23,6,0));  // ### These cancel out.
    QTest::newRow("m(23-12,6),m(10-23,4)")
            << (SignalList() << Move(23,12,6) << Move(10,23,4))
            << (SignalList() << Remove(10,2 ,0) << Remove(21,2,1) << Remove(21,4,2) << Insert(10,4,2) << Insert(23,2,0) << Insert(25,2,1));
    QTest::newRow("m(23-12,6),m(16-23.4)")
            << (SignalList() << Move(23,12,6) << Move(16,23,4))
            << (SignalList() << Remove(12,2,0) << Remove(21,4,1) << Remove(21,2,2) << Insert(12,4,1) << Insert(23,2,2) << Insert(25,2,0));
    QTest::newRow("m(23-12,6),m(13-23,4)")
            << (SignalList() << Move(23,12,6) << Move(13,23,4))
            << (SignalList() << Remove(23,1,0) << Remove(23,4,1) << Remove(23,1,2) << Insert(12,1,0) << Insert(13,1,2) << Insert(23,4,1));
    QTest::newRow("m(23-12,6),m(14-23,)")
            << (SignalList() << Move(23,12,6) << Move(14,23,4))
            << (SignalList() << Remove(23,2,0) << Remove(23,4,1) << Insert(12,2,0) << Insert(23,4,1));
    QTest::newRow("m(23-12,6),m(12-23,4)")
            << (SignalList() << Move(23,12,6) << Move(12,23,4))
            << (SignalList() << Remove(23,4,0) << Remove(23,2,1) << Insert(12,2,1) << Insert(23,4,0));
    QTest::newRow("m(23-12,6),m(11-23,8)")
            << (SignalList() << Move(23,12,6) << Move(11,23,8))
            << (SignalList() << Remove(11,1,0) << Remove(11,1,1) << Remove(21,6,2) << Insert(23,1,0) << Insert(24,6,2) << Insert(30,1,1));
    QTest::newRow("m(23-12,6),m(8-23,4)")
            << (SignalList() << Move(23,12,6) << Move(8,23,4))
            << (SignalList() << Remove(8,4,0) << Remove(19,6,1) << Insert(8,6,1) << Insert(23,4,0));
    QTest::newRow("m(23-12,6),m(2-23,4)")
            << (SignalList() << Move(23,12,6) << Move(2,23,4))
            << (SignalList() << Remove(2,4,0) << Remove(19,6,1) << Insert(8,6,1) << Insert(23,4,0));
    QTest::newRow("m(23-12,6),m(18-23,4)")
            << (SignalList() << Move(23,12,6) << Move(18,23,4))
            << (SignalList() << Remove(12,4,0) << Remove(19,6,1) << Insert(12,6,1) << Insert(23,4,0));
    QTest::newRow("m(23-12,6),m(20-23,4)")
            << (SignalList() << Move(23,12,6) << Move(20,23,4))
            << (SignalList() << Remove(14,4,1) << Remove(19,6,0) << Insert(12,6,0) << Insert(23,4,1));

    QTest::newRow("m(23-12,6),m(11-23,10)")
            << (SignalList() << Move(23,12,6) << Move(11,23,10))
            << (SignalList() << Remove(11,1,3) << Remove(11,3,1) << Remove(19,6,2) << Insert(23,1,3) << Insert(24,6,2) << Insert(30,3,1));

    QTest::newRow("m(3-9,12),m(13-5,12)")
            << (SignalList() << Move(3,9,12) << Move(13,15,5))
            << (SignalList() << Remove(3,4,2) << Remove(3,5,1) << Remove(3,2,0) << Remove(3,1,3) << Insert(9,4,2) << Insert(13,2,0) << Insert(15,5,1) << Insert(20,1,3));
    QTest::newRow("m(3-9,12),m(13-15,20)")
            << (SignalList() << Move(3,9,12) << Move(13,15,20))
            << (SignalList() << Remove(3,4,0) << Remove(3,8,1) << Remove(9,12,2) << Insert(9,4,0) << Insert(15,8,1) << Insert(23,12,2));
    QTest::newRow("m(3-9,12),m(13-15,2)")
            << (SignalList() << Move(3,9,12) << Move(13,15,2))
            << (SignalList() << Remove(3,4,2) << Remove(3,2,1) << Remove(3,2,0) << Remove(3,4,3) << Insert(9,4,2) << Insert(13,2,0) << Insert(15,2,1) << Insert(17,4,3));
    QTest::newRow("m(3-9,12),m(12-5,6)")
            << (SignalList() << Move(3,9,12) << Move(12,5,6))
            << (SignalList() << Remove(3,3,0) << Remove(3,6,1) << Remove(3,3,2) << Insert(5,6,1) << Insert(15,3,0) << Insert(18,3,2));
    QTest::newRow("m(3-9,12),m(10-14,5)")
            << (SignalList() << Move(3,9,12) << Move(10,14,5))
            << (SignalList() << Remove(3,1,2) << Remove(3,5,1) << Remove(3,4,0) << Remove(3,2,3) << Insert(9,1,2) << Insert(10,4,0) << Insert(14,5,1) << Insert(19,2,3));
    QTest::newRow("m(3-9,12),m(16-20,5)")
            << (SignalList() << Move(3,9,12) << Move(16,20,5))
            << (SignalList() << Remove(3,7,0) << Remove(3,5,1) << Insert(9,7,0) << Insert(20,5,1));
    QTest::newRow("m(3-9,12),m(13-17,5)")
            << (SignalList() << Move(3,9,12) << Move(13,17,5))
            << (SignalList() << Remove(3,4,0) << Remove(3,5,1) << Remove(3,3,2) << Insert(9,4,0) << Insert(13,3,2) << Insert(17,5,1));
    QTest::newRow("m(3-9,12),m(14-18,5)")
            << (SignalList() << Move(3,9,12) << Move(14,18,5))
            << (SignalList() << Remove(3,5,0) << Remove(3,5,1) << Remove(3,2,2) << Insert(9,5,0) << Insert(14,2,2) << Insert(18,5,1));
    QTest::newRow("m(3-9,12),m(12-16,5)")
            << (SignalList() << Move(3,9,12) << Move(12,16,5))
            << (SignalList() << Remove(3,3,2) << Remove(3,5,1) << Remove(3,4,0) << Insert(9,3,2) << Insert(12,4,0) << Insert(16,5,1));
    QTest::newRow("m(3-9,12),m(11-19,5)")
            << (SignalList() << Move(3,9,12) << Move(11,19,5))
            << (SignalList() << Remove(3,2,0) << Remove(3,5,1) << Remove(3,5,2) << Insert(9,2,0) << Insert(11,5,2) << Insert(19,5,1));
    QTest::newRow("m(3-9,12),m(8-12,5)")
            << (SignalList() << Move(3,9,12) << Move(8,12,5))
            << (SignalList() << Remove(3,4,1) << Remove(3,4,0) << Remove(3,4,4) << Remove(8,1,2) << Insert(8,4,0) << Insert(12,1,2) << Insert(13,4,1) << Insert(17,4,4));
    QTest::newRow("m(3-9,12),m(2-6,5)")
            << (SignalList() << Move(3,9,12) << Move(2,6,5))
            << (SignalList() <<  Remove(2,1,2) << Remove(2,2,0) << Remove(2,10,3) << Remove(2,4,1) << Insert(4,2,0) << Insert(6,1,2) << Insert(7,4,1) << Insert(11,10,3));
    QTest::newRow("m(3-9,12),m(18-22,5)")
            << (SignalList() << Move(3,9,12) << Move(18,22,5))
            << (SignalList() << Remove(3,9,0) << Remove(3,3,1) << Remove(9,2,2) << Insert(9,9,0) << Insert(22,3,1) << Insert(25,2,2));
    QTest::newRow("m(3-9,12),m(20-24,5)")
            << (SignalList() << Move(3,9,12) << Move(20,24,5))
            << (SignalList() << Remove(3,11,0) << Remove(3,1,1) << Remove(9,4,2) << Insert(9,11,0) << Insert(24,1,1) << Insert(25,4,2));

    QTest::newRow("m(3-9,12),m(5-11,8)")
            << (SignalList() << Move(3,9,12) << Move(5,11,8))
            << (SignalList() << Remove(3,4,1) << Remove(3,6,0) << Remove(3,2,3) << Remove(5,4,2) << Insert(5,6,0) << Insert(11,4,2) << Insert(15,2,3) << Insert(15,4,1));

    QTest::newRow("m(3-9,12),m(12-23,6)")
            << (SignalList() << Move(3,9,12) << Move(12,23,6))
            << (SignalList() << Remove(3,3,2) << Remove(3,6,1) << Remove(3,3,0) << Insert(9,3,2) << Insert(12,3,0) << Insert(23,6,1));
    QTest::newRow("m(3-9,12),m(10-23,4)")
            << (SignalList() << Move(3,9,12) << Move(10,23,4))
            << (SignalList() << Remove(3,1,2) << Remove(3,4,1) << Remove(3,7,0) << Insert(9,1,2) << Insert(10,7,0) << Insert(23,4,1));
    QTest::newRow("m(3-9,12),m(16-23,4)")
            << (SignalList() << Move(3,9,12) << Move(16,23,4))
            << (SignalList() << Remove(3,7,2) << Remove(3,4,1) << Remove(3,1,0) << Insert(9,7,2) << Insert(16,1,0) << Insert(23,4,1));
    QTest::newRow("m(3-9,12),m(13-23,4)")
            << (SignalList() << Move(3,9,12) << Move(13,23,4))
            << (SignalList() << Remove(3,4,2) << Remove(3,4,1) << Remove(3,4,0) << Insert(9,4,2) << Insert(13,4,0) << Insert(23,4,1));
    QTest::newRow("m(3-9,12),m(14-23,4)")
            << (SignalList() << Move(3,9,12) << Move(14,23,4))
            << (SignalList() << Remove(3,5,2) << Remove(3,4,1) << Remove(3,3,0) << Insert(9,5,2) << Insert(14,3,0) << Insert(23,4,1));
    QTest::newRow("m(3-9,12),m(12-23,4)")
            << (SignalList() << Move(3,9,12) << Move(12,23,4))
            << (SignalList() << Remove(3,3,2) << Remove(3,4,1) << Remove(3,5,0) << Insert(9,3,2) << Insert(12,5,0) << Insert(23,4,1));
    QTest::newRow("m(3-9,12),m(11-23,8)")
            << (SignalList() << Move(3,9,12) << Move(11,23,8))
            << (SignalList() << Remove(3,2,2) << Remove(3,8,1) << Remove(3,2,0) << Insert(9,2,2) << Insert(11,2,0) << Insert(23,8,1));
    QTest::newRow("m(3-9,12),m(8-23,4)")
            << (SignalList() << Move(3,9,12) << Move(8,23,4))
            << (SignalList() << Remove(3,3,1) << Remove(3,9,0) << Remove(8,1,2) << Insert(8,9,0) << Insert(23,1,2) << Insert(24,3,1));
    QTest::newRow("m(3-9,12),m(2-23,4)")
            << (SignalList() << Move(3,9,12) << Move(2,23,4))
            << (SignalList() << Remove(2,1,2) << Remove(2,12,0) << Remove(2,3,1) << Insert(5,12,0) << Insert(23,1,2) << Insert(24,3,1));
    QTest::newRow("m(3-9,12),m(18-23,4)")
            << (SignalList() << Move(3,9,12) << Move(18,23,4))
            << (SignalList() << Remove(3,9,3) << Remove(3,3,2) << Remove(9,1,1) << Insert(9,9,3) << Insert(23,3,2) << Insert(26,1,1));
    QTest::newRow("m(3-9,12),m(20-23,4)")
            << (SignalList() << Move(3,9,12) << Move(20,23,4))
            << (SignalList() << Remove(3,11,3) << Remove(3,1,2) << Remove(9,3,1) << Insert(9,11,3) << Insert(23,1,2) << Insert(24,3,1));

    QTest::newRow("m(3-9,12),m(11-23,10)")
            << (SignalList() << Move(3,9,12) << Move(11,23,10))
            << (SignalList() << Remove(3,2,2) << Remove(3,10,1) << Insert(9,2,2) << Insert(23,10,1));

    // Change
    QTest::newRow("c(4,5)")
            << (SignalList() << Change(4,5))
            << (SignalList() << Change(4,5));
    QTest::newRow("c(4,5),c(12,2)")
            << (SignalList() << Change(4,5) << Change(12,2))
            << (SignalList() << Change(4,5) << Change(12,2));
    QTest::newRow("c(12,2),c(4,5)")
            << (SignalList() << Change(12,2) << Change(4,5))
            << (SignalList() << Change(4,5) << Change(12,2));
    QTest::newRow("c(4,5),c(2,2)")
            << (SignalList() << Change(4,5) << Change(2,2))
            << (SignalList() << Change(2,7));
    QTest::newRow("c(4,5),c(9,2)")
            << (SignalList() << Change(4,5) << Change(9,2))
            << (SignalList() << Change(4,7));
    QTest::newRow("c(4,5),c(3,2)")
            << (SignalList() << Change(4,5) << Change(3,2))
            << (SignalList() << Change(3,6));
    QTest::newRow("c(4,5),c(8,2)")
            << (SignalList() << Change(4,5) << Change(8,2))
            << (SignalList() << Change(4,6));
    QTest::newRow("c(4,5),c(3,2)")
            << (SignalList() << Change(4,5) << Change(3,2))
            << (SignalList() << Change(3,6));
    QTest::newRow("c(4,5),c(2,9)")
            << (SignalList() << Change(4,5) << Change(2,9))
            << (SignalList() << Change(2,9));
    QTest::newRow("c(4,5),c(12,3),c(8,6)")
            << (SignalList() << Change(4,5) << Change(12,3) << Change(8,6))
            << (SignalList() << Change(4,11));

    // Insert,then remove.
    QTest::newRow("i(12,6),r(12,6)")
            << (SignalList() << Insert(12,6) << Remove(12,6))
            << (SignalList());
    QTest::newRow("i(12,6),r(10,4)")
            << (SignalList() << Insert(12,6) << Remove(10,4))
            << (SignalList() << Remove(10,2) << Insert(10,4));
    QTest::newRow("i(12,6),r(16,4)")
            << (SignalList() << Insert(12,6) << Remove(16,4))
            << (SignalList() << Remove(12,2) << Insert(12,4));
    QTest::newRow("i(12,6),r(13,4)")
            << (SignalList() << Insert(12,6) << Remove(13,4))
            << (SignalList() << Insert(12,2));
    QTest::newRow("i(12,6),r(14,4)")
            << (SignalList() << Insert(12,6) << Remove(14,4))
            << (SignalList() << Insert(12,2));
    QTest::newRow("i(12,6),r(12,4)")
            << (SignalList() << Insert(12,6) << Remove(12,4))
            << (SignalList() << Insert(12,2));
    QTest::newRow("i(12,6),r(11,8)")
            << (SignalList() << Insert(12,6) << Remove(11,8))
            << (SignalList() << Remove(11,2));
    QTest::newRow("i(12,6),r(8,4)")
            << (SignalList() << Insert(12,6) << Remove(8,4))
            << (SignalList() << Remove(8,4) << Insert(8,6));
    QTest::newRow("i(12,6),r(2,4)")
            << (SignalList() << Insert(12,6) << Remove(2,4))
            << (SignalList() << Remove(2,4) << Insert(8,6));
    QTest::newRow("i(12,6),r(18,4)")
            << (SignalList() << Insert(12,6) << Remove(18,4))
            << (SignalList() << Remove(12,4) << Insert(12,6));
    QTest::newRow("i(12,6),r(20,4)")
            << (SignalList() << Insert(12,6) << Remove(20,4))
            << (SignalList() << Remove(14,4) << Insert(12,6));

    // Insert,then move
    QTest::newRow("i(12,6),m(12-5,6)")
            << (SignalList() << Insert(12,6) << Move(12,5,6))
            << (SignalList() << Insert(5,6));
    QTest::newRow("i(12,6),m(10-5,4)")
            << (SignalList() << Insert(12,6) << Move(10,5,4))
            << (SignalList() << Remove(10,2,0) << Insert(5,2,0) << Insert(7,2) << Insert(14,4));
    QTest::newRow("i(12,6),m(16-5,4)")
            << (SignalList() << Insert(12,6) << Move(16,5,4))
            << (SignalList() << Remove(12,2,0) << Insert(5,2) << Insert(7,2,0) << Insert(16,4));
    QTest::newRow("i(12,6),m(13-5,4)")
            << (SignalList() << Insert(12,6) << Move(13,5,4))
            << (SignalList() << Insert(5,4) << Insert(16,2));
    QTest::newRow("i(12,6),m(14-5,4)")
            << (SignalList() << Insert(12,6) << Move(14,5,4))
            << (SignalList() << Insert(5,4) << Insert(16,2));
    QTest::newRow("i(12,6),m(12-5,4)")
            << (SignalList() << Insert(12,6) << Move(12,5,4))
            << (SignalList() << Insert(5,4) << Insert(16,2));
    QTest::newRow("i(12,6),m(11-5,8)")
            << (SignalList() << Insert(12,6) << Move(11,5,8))
            << (SignalList() << Remove(11,1,0) << Remove(11,1,1) << Insert(5,1,0) << Insert(6,6) << Insert(12,1,1));
    QTest::newRow("i(12,6),m(8-5,4)")
            << (SignalList() << Insert(12,6) << Move(8,5,4))
            << (SignalList() << Remove(8,4,0) << Insert(5,4,0) << Insert(12,6));
    QTest::newRow("i(12,6),m(2-5,4)")
            << (SignalList() << Insert(12,6) << Move(2,5,4))
            << (SignalList() << Remove(2,4,0) << Insert(5,4,0) << Insert(12,6));
    QTest::newRow("i(12,6),m(18-5,4)")
            << (SignalList() << Insert(12,6) << Move(18,5,4))
            << (SignalList() << Remove(12,4,0) << Insert(5,4,0) << Insert(16,6));
    QTest::newRow("i(12,6),m(20-5,4)")
            << (SignalList() << Insert(12,6) << Move(20,5,4))
            << (SignalList() << Remove(14,4,0) << Insert(5,4,0) << Insert(16,6));

    QTest::newRow("i(12,6),m(5-13,11)")
            << (SignalList() << Insert(12,6) << Move(5,11,8))
            << (SignalList() << Remove(5,7,0) << Insert(5,5) << Insert(11,7,0) << Insert(18,1));

    QTest::newRow("i(12,6),m(12-23,6)")
            << (SignalList() << Insert(12,6) << Move(12,23,6))
            << (SignalList() << Insert(23,6));
    QTest::newRow("i(12,6),m(10-23,4)")
            << (SignalList() << Insert(12,6) << Move(10,23,4))
            << (SignalList() << Remove(10,2,0) << Insert(10,4) << Insert(23,2,0) << Insert(25,2));
    QTest::newRow("i(12,6),m(16-23,4)")
            << (SignalList() << Insert(12,6) << Move(16,23,4))
            << (SignalList() << Remove(12,2,0) << Insert(12,4) << Insert(23,2) << Insert(25,2,0));
    QTest::newRow("i(12,6),m(13-23,4)")
            << (SignalList() << Insert(12,6) << Move(13,23,4))
            << (SignalList() << Insert(12,2) << Insert(23,4));
    QTest::newRow("i(12,6),m(14-23,4)")
            << (SignalList() << Insert(12,6) << Move(14,23,4))
            << (SignalList() << Insert(12,2) << Insert(23,4));
    QTest::newRow("i(12,6),m(12-23,4)")
            << (SignalList() << Insert(12,6) << Move(12,23,4))
            << (SignalList() << Insert(12,2) << Insert(23,4));
    QTest::newRow("i(12,6),m(11-23,8)")
            << (SignalList() << Insert(12,6) << Move(11,23,8))
            << (SignalList() << Remove(11,1,0) << Remove(11,1,1) << Insert(23,1,0)<< Insert(24,6) << Insert(30,1,1));
    QTest::newRow("i(12,6),m(8-23,4)")
            << (SignalList() << Insert(12,6) << Move(8,23,4))
            << (SignalList() << Remove(8,4,0) << Insert(8,6) << Insert(23,4,0));
    QTest::newRow("i(12,6),m(2-23,4)")
            << (SignalList() << Insert(12,6) << Move(2,23,4))
            << (SignalList() << Remove(2,4,0) << Insert(8,6) << Insert(23,4,0));
    QTest::newRow("i(12,6),m(18-23,4)")
            << (SignalList() << Insert(12,6) << Move(18,23,4))
            << (SignalList() << Remove(12,4,0) << Insert(12,6) << Insert(23,4,0));
    QTest::newRow("i(12,6),m(20-23,4)")
            << (SignalList() << Insert(12,6) << Move(20,23,4))
            << (SignalList() << Remove(14,4,0) << Insert(12,6) << Insert(23,4,0));

    QTest::newRow("i(12,6),m(11-23,10)")
            << (SignalList() << Insert(12,6) << Move(11,23,10))
            << (SignalList() << Remove(11,1,0) << Remove(11,3,1) << Insert(23,1,0) << Insert(24,6) << Insert(30,3,1));

    // Insert,then change
    QTest::newRow("i(12,6),c(12,6)")
            << (SignalList() << Insert(12,6) << Change(12,6))
            << (SignalList() << Insert(12,6));
    QTest::newRow("i(12,6),c(10,6)")
            << (SignalList() << Insert(12,6) << Change(10,6))
            << (SignalList() << Insert(12,6) << Change(10,2));
    QTest::newRow("i(12,6),c(16,4)")
            << (SignalList() << Insert(12,6) << Change(16,4))
            << (SignalList() << Insert(12,6) << Change(18,2));
    QTest::newRow("i(12,6),c(13,4)")
            << (SignalList() << Insert(12,6) << Change(13,4))
            << (SignalList() << Insert(12,6));
    QTest::newRow("i(12,6),c(14,4)")
            << (SignalList() << Insert(12,6) << Change(14,4))
            << (SignalList() << Insert(12,6));
    QTest::newRow("i(12,6),c(12,4)")
            << (SignalList() << Insert(12,6) << Change(12,4))
            << (SignalList() << Insert(12,6));
    QTest::newRow("i(12,6),c(11,8)")
            << (SignalList() << Insert(12,6) << Change(11,8))
            << (SignalList() << Insert(12,6) << Change(11,1) << Change(18,1));
    QTest::newRow("i(12,6),c(8,4)")
            << (SignalList() << Insert(12,6) << Change(8,4))
            << (SignalList() << Insert(12,6) << Change(8,4));
    QTest::newRow("i(12,6),c(2,4)")
            << (SignalList() << Insert(12,6) << Change(2,4))
            << (SignalList() << Insert(12,6) << Change(2,4));
    QTest::newRow("i(12,6),c(18,4)")
            << (SignalList() << Insert(12,6) << Change(18,4))
            << (SignalList() << Insert(12,6) << Change(18,4));
    QTest::newRow("i(12,6),c(20,4)")
            << (SignalList() << Insert(12,6) << Change(20,4))
            << (SignalList() << Insert(12,6) << Change(20,4));

    // Remove,then insert
    QTest::newRow("r(12,6),i(12,6)")
            << (SignalList() << Remove(12,6) << Insert(12,6))
            << (SignalList() << Remove(12,6) << Insert(12,6));
    QTest::newRow("r(12,6),i(10,4)")
            << (SignalList() << Remove(12,6) << Insert(10,14))
            << (SignalList() << Remove(12,6) << Insert(10,14));
    QTest::newRow("r(12,6),i(16,4)")
            << (SignalList() << Remove(12,6) << Insert(16,4))
            << (SignalList() << Remove(12,6) << Insert(16,4));
    QTest::newRow("r(12,6),i(13,4)")
            << (SignalList() << Remove(12,6) << Insert(13,4))
            << (SignalList() << Remove(12,6) << Insert(13,4));
    QTest::newRow("r(12,6),i(14,4)")
            << (SignalList() << Remove(12,6) << Insert(14,4))
            << (SignalList() << Remove(12,6) << Insert(14,4));
    QTest::newRow("r(12,6),i(12,4)")
            << (SignalList() << Remove(12,6) << Insert(12,4))
            << (SignalList() << Remove(12,6) << Insert(12,4));
    QTest::newRow("r(12,6),i(11,8)")
            << (SignalList() << Remove(12,6) << Insert(11,8))
            << (SignalList() << Remove(12,6) << Insert(11,8));
    QTest::newRow("r(12,6),i(8,4)")
            << (SignalList() << Remove(12,6) << Insert(8,4))
            << (SignalList() << Remove(12,6) << Insert(8,4));
    QTest::newRow("r(12,6),i(2,4)")
            << (SignalList() << Remove(12,6) << Insert(2,4))
            << (SignalList() << Remove(12,6) << Insert(2,4));
    QTest::newRow("r(12,6),i(18,4)")
            << (SignalList() << Remove(12,6) << Insert(18,4))
            << (SignalList() << Remove(12,6) << Insert(18,4));
    QTest::newRow("r(12,6),i(20,4)")
            << (SignalList() << Remove(12,6) << Insert(20,4))
            << (SignalList() << Remove(12,6) << Insert(20,4));

    // Move,then insert
    QTest::newRow("m(12-5,6),i(12,6)")
            << (SignalList() << Move(12,5,6) << Insert(12,6))
            << (SignalList() << Remove(12,6,0) << Insert(5,6,0) << Insert(12,6));
    QTest::newRow("m(12-5,6),i(10,4)")
            << (SignalList() << Move(12,5,6) << Insert(10,4))
            << (SignalList() << Remove(12,5,0) << Remove(12,1,1) << Insert(5,5,0) << Insert(10,4) << Insert(14,1,1));
    QTest::newRow("m(12-5,6),i(16,4)")
            << (SignalList() << Move(12,5,6) << Insert(16,4))
            << (SignalList() << Remove(12,6,0) << Insert(5,6,0) << Insert(16,4));
    QTest::newRow("m(12-5,6),i(13,4)")
            << (SignalList() << Move(12,5,6) << Insert(13,4))
            << (SignalList() << Remove(12,6,0) << Insert(5,6,0) << Insert(13,4));
    QTest::newRow("m(12-5,6),i(14,4)")
            << (SignalList() << Move(12,5,6) << Insert(14,4))
            << (SignalList() << Remove(12,6,0) << Insert(5,6,0) << Insert(14,4));
    QTest::newRow("m(12-5,6),i(12,4)")
            << (SignalList() << Move(12,5,6) << Insert(12,4))
            << (SignalList() << Remove(12,6,0) << Insert(5,6,0) << Insert(12,4));
    QTest::newRow("m(12-5,6),i(11,8)")
            << (SignalList() << Move(12,5,6) << Insert(11,8))
            << (SignalList() << Remove(12,6,0) << Insert(5,6,0) << Insert(11,8));
    QTest::newRow("m(12-5,6),i(8,4)")
            << (SignalList() << Move(12,5,6) << Insert(8,4))
            << (SignalList() << Remove(12,3,0) << Remove(12,3,1) << Insert(5,3,0) << Insert(8,4) << Insert(12,3,1));
    QTest::newRow("m(12-5,6),i(2,4)")
            << (SignalList() << Move(12,5,6) << Insert(2,4))
            << (SignalList() << Remove(12,6,0) << Insert(2,4) << Insert(9,6,0));
    QTest::newRow("m(12-5,6),i(18,4)")
            << (SignalList() << Move(12,5,6) << Insert(18,4))
            << (SignalList() << Remove(12,6,0) << Insert(5,6,0) << Insert(18,4));
    QTest::newRow("m(12-5,6),i(20,4)")
            << (SignalList() << Move(12,5,6) << Insert(20,4))
            << (SignalList() << Remove(12,6,0) << Insert(5,6,0) << Insert(20,4));

    QTest::newRow("m(12-23,6),i(12,6)")
            << (SignalList() << Move(12,23,6) << Insert(12,6))
            << (SignalList() << Remove(12,6,0) << Insert(12,6) << Insert(29,6,0));
    QTest::newRow("m(12-23,6),i(10,4)")
            << (SignalList() << Move(12,23,6) << Insert(10,4))
            << (SignalList() << Remove(12,6,0) << Insert(10,4) << Insert(27,6,0));
    QTest::newRow("m(12-23,6),i(16,4)")
            << (SignalList() << Move(12,23,6) << Insert(16,4))
            << (SignalList() << Remove(12,6,0) << Insert(16,4) << Insert(27,6,0));
    QTest::newRow("m(12-23,6),i(13,4)")
            << (SignalList() << Move(12,23,6) << Insert(13,4))
            << (SignalList() << Remove(12,6,0) << Insert(13,4) << Insert(27,6,0));
    QTest::newRow("m(12-23,6),i(14,4)")
            << (SignalList() << Move(12,23,6) << Insert(14,4))
            << (SignalList() << Remove(12,6,0) << Insert(14,4) << Insert(27,6,0));
    QTest::newRow("m(12-23,6),i(12,4)")
            << (SignalList() << Move(12,23,6) << Insert(12,4))
            << (SignalList() << Remove(12,6,0) << Insert(12,4) << Insert(27,6,0));
    QTest::newRow("m(12-23,6),i(11,8)")
            << (SignalList() << Move(12,23,6) << Insert(11,8))
            << (SignalList() << Remove(12,6,0) << Insert(11,8) << Insert(31,6,0));
    QTest::newRow("m(12-23,6),i(8,4)")
            << (SignalList() << Move(12,23,6) << Insert(8,4))
            << (SignalList() << Remove(12,6,0) << Insert(8,4) << Insert(27,6,0));
    QTest::newRow("m(12-23,6),i(2,4)")
            << (SignalList() << Move(12,23,6) << Insert(2,4))
            << (SignalList() << Remove(12,6,0) << Insert(2,4) << Insert(27,6,0));
    QTest::newRow("m(12-23,6),i(18,4)")
            << (SignalList() << Move(12,23,6) << Insert(18,4))
            << (SignalList() << Remove(12,6,0) << Insert(18,4) << Insert(27,6,0));
    QTest::newRow("m(12-23,6),i(20,4)")
            << (SignalList() << Move(12,23,6) << Insert(20,4))
            << (SignalList() << Remove(12,6,0) << Insert(20,4) << Insert(27,6,0));

    // Move,then remove
    QTest::newRow("m(12-5,6),r(12,6)")
            << (SignalList() << Move(12,5,6) << Remove(12,6))
            << (SignalList() << Remove(6,6) << Remove(6,6,0) << Insert(5,6,0));
    QTest::newRow("m(12-5,6),r(10,4)")
            << (SignalList() << Move(12,5,6) << Remove(10,4))
            << (SignalList() << Remove(5,3) << Remove(9,5,0) << Remove(9,1) << Insert(5,5,0));
    QTest::newRow("m(12-5,6),r(16,4)")
            << (SignalList() << Move(12,5,6) << Remove(16,4))
            << (SignalList() << Remove(10,2) << Remove(10,6,0) << Remove(10,2) << Insert(5,6,0));
    QTest::newRow("m(12-5,6),r(13,4)")
            << (SignalList() << Move(12,5,6) << Remove(13,4))
            << (SignalList() << Remove(7,4) << Remove(8,6,0) << Insert(5,6,0));
    QTest::newRow("m(12-5,6),r(14,4)")
            << (SignalList() << Move(12,5,6) << Remove(14,4))
            << (SignalList() << Remove(8,4) << Remove(8,6,0) << Insert(5,6,0));
    QTest::newRow("m(12-5,6),r(12,4)")
            << (SignalList() << Move(12,5,6) << Remove(12,4))
            << (SignalList() << Remove(6,4) << Remove(8,6,0) << Insert(5,6,0));
    QTest::newRow("m(12-5,6),r(11,8)")
            << (SignalList() << Move(12,5,6) << Remove(11,8))
            << (SignalList() << Remove(5,7) << Remove(5,6,0) << Remove(5,1) << Insert(5,6,0));
    QTest::newRow("m(12-5,6),r(8,4)")
            << (SignalList() << Move(12,5,6) << Remove(8,4))
            << (SignalList() << Remove(5,1) << Remove(11,3,0) << Remove(11,3) << Insert(5,3,0));
    QTest::newRow("m(12-5,6),r(2,4)")
            << (SignalList() << Move(12,5,6) << Remove(2,4))
            << (SignalList() << Remove(2,3) << Remove(9,1) << Remove(9,5,0) << Insert(2,5,0));
    QTest::newRow("m(12-5,6),r(6,4)")
            << (SignalList() << Move(12,5,6) << Remove(6,4))
            << (SignalList() << Remove(12,1,0) << Remove(12,4) << Remove(12,1,1) << Insert(5,1,0) << Insert(6,1,1));
    QTest::newRow("m(12-5,6),r(18,4)")
            << (SignalList() << Move(12,5,6) << Remove(18,4))
            << (SignalList() << Remove(12,6,0) << Remove(12,4) << Insert(5,6,0));
    QTest::newRow("m(12-5,6),r(20,4)")
            << (SignalList() << Move(12,5,6) << Remove(20,4))
            << (SignalList() << Remove(12,6,0) << Remove(14,4) << Insert(5,6,0));

    QTest::newRow("m(12-23,6),r(12,6)")
            << (SignalList() << Move(12,23,6) << Remove(12,6))
            << (SignalList() << Remove(12,6,0) << Remove(12,6) << Insert(17,6,0));
    QTest::newRow("m(12-23,6),r(10,4)")
            << (SignalList() << Move(12,23,6) << Remove(10,4))
            << (SignalList() << Remove(10,2) << Remove(10,6,0) << Remove(10,2) << Insert(19,6,0));
    QTest::newRow("m(12-23,6),r(16,4)")
            << (SignalList() << Move(12,23,6) << Remove(16,4))
            << (SignalList() << Remove(12,6,0) << Remove(16,4) << Insert(19,6,0));
    QTest::newRow("m(12-23,6),r(13,4)")
            << (SignalList() << Move(12,23,6) << Remove(13,4))
            << (SignalList() << Remove(12,6,0) << Remove(13,4) << Insert(19,6,0));
    QTest::newRow("m(12-23,6),r(14,4)")
            << (SignalList() << Move(12,23,6) << Remove(14,4))
            << (SignalList() << Remove(12,6,0) << Remove(14,4) << Insert(19,6,0));
    QTest::newRow("m(12-23,6),r(12,4)")
            << (SignalList() << Move(12,23,6) << Remove(12,4))
            << (SignalList() << Remove(12,6,0) << Remove(12,4) << Insert(19,6,0));
    QTest::newRow("m(12-23,6),r(11,8)")
            << (SignalList() << Move(12,23,6) << Remove(11,8))
            << (SignalList() << Remove(11,1) << Remove(11,6,0) << Remove(11,7) << Insert(15,6,0));
    QTest::newRow("m(12-23,6),r(8,4)")
            << (SignalList() << Move(12,23,6) << Remove(8,4))
            << (SignalList() << Remove(8,4) << Remove(8,6,0) << Insert(19,6,0));
    QTest::newRow("m(12-23,6),r(2,4)")
            << (SignalList() << Move(12,23,6) << Remove(2,4))
            << (SignalList() << Remove(2,4) << Remove(8,6,0) << Insert(19,6,0));
    QTest::newRow("m(12-23,6),r(18,4)")
            << (SignalList() << Move(12,23,6) << Remove(18,4))
            << (SignalList() << Remove(12,6,0) << Remove(18,4) << Insert(19,6,0));
    QTest::newRow("m(12-23,6),r(20,4)")
            << (SignalList() << Move(12,23,6) << Remove(20,4))
            << (SignalList() << Remove(12,1) << Remove(12,5,0) << Remove(20,3) << Insert(20,5,0));


    // Complex
    QTest::newRow("r(15,1),r(22,1)")
            << (SignalList() << Remove(15,1) << Remove(22,1))
            << (SignalList() << Remove(15,1) << Remove(22,1));
    QTest::newRow("r(15,1),r(22,1),r(25,1)")
            << (SignalList() << Remove(15,1) << Remove(22,1) << Remove(25,1))
            << (SignalList() << Remove(15,1) << Remove(22,1) << Remove(25,1));
    QTest::newRow("r(15,1),r(22,1),r(25,1),r(15,1)")
            << (SignalList() << Remove(15,1) << Remove(22,1) << Remove(25,1) << Remove(15,1))
            << (SignalList() << Remove(15,2) << Remove(21,1) << Remove(24,1));
    QTest::newRow("r(15,1),r(22,1),r(25,1),r(15,1),r(13,1)")
            << (SignalList() << Remove(15,1) << Remove(22,1) << Remove(25,1) << Remove(15,1) << Remove(13,1))
            << (SignalList() << Remove(13,1) << Remove(14,2) << Remove(20,1) << Remove(23,1));
    QTest::newRow("r(15,1),r(22,1),r(25,1),r(15,1),r(13,1),r(13,1)")
            << (SignalList() << Remove(15,1) << Remove(22,1) << Remove(25,1) << Remove(15,1) << Remove(13,1) << Remove(13,1))
            << (SignalList() << Remove(13,4) << Remove(19,1) << Remove(22,1));
    QTest::newRow("r(15,1),r(22,1),r(25,1),r(15,1),r(13,1),r(13,1),m(12,13,1)")
            << (SignalList() << Remove(15,1) << Remove(22,1) << Remove(25,1) << Remove(15,1) << Remove(13,1) << Remove(13,1) << Move(12,13,1))
            << (SignalList() << Remove(12,1,0) << Remove(12,4) << Remove(18,1) << Remove(21,1) << Insert(13,1,0));

}

void tst_qdeclarativemodelchange::sequence()
{
    QFETCH(SignalList, input);
    QFETCH(SignalList, output);

    QDeclarativeChangeSet set;

    foreach (const Signal &signal, input) {
        if (signal.isRemove())
            set.remove(signal.index, signal.count);
        else if (signal.isInsert())
            set.insert(signal.index, signal.count);
        else if (signal.isMove())
            set.move(signal.index, signal.to, signal.count);
        else if (signal.isChange())
            set.change(signal.index, signal.count);
    }

    SignalList changes;
    foreach (const QDeclarativeChangeSet::Remove &remove, set.removes())
        changes << Remove(remove.index, remove.count, remove.moveId);
    foreach (const QDeclarativeChangeSet::Insert &insert, set.inserts())
        changes << Insert(insert.index, insert.count, insert.moveId);
    foreach (const QDeclarativeChangeSet::Change &change, set.changes())
        changes << Change(change.index, change.count);

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
