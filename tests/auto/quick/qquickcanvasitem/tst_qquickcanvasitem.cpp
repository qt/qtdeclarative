// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#include <QtQuickTest/quicktest.h>
#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlcontext.h>

class Setup : public QObject
{
    Q_OBJECT

public:
    Setup() {}

public slots:
    void qmlEngineAvailable(QQmlEngine *engine)
    {
        engine->rootContext()->setContextProperty("hasImageFormats", QVariant(
#ifdef HAS_IMAGE_FORMATS
            true
#else
            false
#endif
            ));
        engine->rootContext()->setContextProperty("applicationDirPath",
                                                  QCoreApplication::applicationDirPath());
    }
};

QUICK_TEST_MAIN_WITH_SETUP(qquickcanvasitem, Setup)

#include "tst_qquickcanvasitem.moc"
