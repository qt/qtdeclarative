// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <private/qqmljscompiler_p.h>

#include <QLibraryInfo>
#include <QTest>

class HugeArrayCompilation : public QObject
{
    Q_OBJECT

private slots:
    void compile();
};

void HugeArrayCompilation::compile()
{
    QQmlJSResourceFileMapper mapper({});
    QQmlJSImporter importer({ QLibraryInfo::path(QLibraryInfo::QmlImportsPath) }, &mapper);
    QQmlJSLogger logger;
    logger.setFilePath("file.qml");

    const QString path = QLatin1String(SRCDIR) + QLatin1String("/data/Array.qml");
    QQmlJSSaveFunction saveFunction = [](const QV4::CompiledData::SaveableUnitPointer &,
                                         const QQmlJSAotFunctionMap &, const LookupSignatures &,
                                         QString *) -> bool { return true; };
    QQmlJSAotCompiler compiler(&importer, {}, {}, &logger);
    QQmlJSCompileError error;

    QBENCHMARK {
        (void) qCompileQmlFile(path, saveFunction, &compiler, &error);
    }
}

QTEST_MAIN(HugeArrayCompilation)

#include "tst_hugeArrayCompilation.moc"
