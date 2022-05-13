// Copyright (C) 2017 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Milian Wolff <milian.wolff@kdab.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <qtest.h>

#include <QDebug>

#include <private/qqmlchangeset_p.h>

class tst_qqmlchangeset : public QObject
{
    Q_OBJECT

private slots:
    void move();
};

void tst_qqmlchangeset::move()
{
    QBENCHMARK {
        QQmlChangeSet set;
        const int MAX_ROWS = 30000;
        for (int i = 0; i < MAX_ROWS; ++i) {
            set.move(i, MAX_ROWS - 1 - i, 1, i);
        }
    }
}

QTEST_MAIN(tst_qqmlchangeset)
#include "tst_qqmlchangeset.moc"
