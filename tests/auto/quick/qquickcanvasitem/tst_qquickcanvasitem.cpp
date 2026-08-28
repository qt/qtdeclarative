// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
#include <QtQuickTest/quicktest.h>
#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlcontext.h>
#include <QtCore/qtemporarydir.h>
#include <QtCore/qurl.h>

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
        const QString imagePath = m_temporaryDir.filePath(QStringLiteral("c.png"));
        const QString sizedImagePath = m_temporaryDir.filePath(QStringLiteral("c2.png"));
        engine->rootContext()->setContextProperty("temporaryImagePath", imagePath);
        engine->rootContext()->setContextProperty("temporaryImageUrl",
                                                  QUrl::fromLocalFile(imagePath));
        engine->rootContext()->setContextProperty("temporarySizedImagePath", sizedImagePath);
        engine->rootContext()->setContextProperty("temporarySizedImageUrl",
                                                  QUrl::fromLocalFile(sizedImagePath));
    }

private:
    QTemporaryDir m_temporaryDir;
};

QUICK_TEST_MAIN_WITH_SETUP(qquickcanvasitem, Setup)

#include "tst_qquickcanvasitem.moc"
