// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "stringdumper/tst_qmldomstringdumper.h"
#include "errormessage/tst_qmldomerrormessage.h"
#include "domitem/tst_qmldomitem.h"
#include "merging/tst_dommerging.h"
#include "path/tst_qmldompath.h"
#include "reformatter/tst_reformatter.h"

#include <QtCore/qdebug.h>

int main(int argc, char *argv[])
{
    int status = 0;
    {
        QQmlJS::Dom::TestStringDumper test;
        status |= QTest::qExec(&test, argc, argv);
    }
    {
        QQmlJS::Dom::PathEls::TestPaths test;
        status |= QTest::qExec(&test, argc, argv);
    }
    {
        QQmlJS::Dom::TestErrorMessage test;
        status |= QTest::qExec(&test, argc, argv);
    }
    {
        QQmlJS::Dom::TestDomItem test;
        status |= QTest::qExec(&test, argc, argv);
    }
    {
        QQmlJS::Dom::TestDomMerging test;
        status |= QTest::qExec(&test, argc, argv);
    }
    {
        QQmlJS::Dom::TestReformatter test;
        status |= QTest::qExec(&test, argc, argv);
    }
    if (status)
        qWarning() << "Combined test failed!";
    return status;
}
