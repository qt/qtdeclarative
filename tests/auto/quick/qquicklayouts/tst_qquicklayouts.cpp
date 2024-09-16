// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlcontext.h>
#include <QtQuickTest/quicktest.h>

class Setup : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool bindingLoopDetected READ wasBindingLoopDetected FINAL)
    Q_PROPERTY(bool useDefaultSizePolicy READ useDefaultSizePolicy WRITE setUseDefaultSizePolicy FINAL)

public:
    Setup() {}

    bool wasBindingLoopDetected() const { return mBindingLoopDetected; }

    bool useDefaultSizePolicy() const { return QCoreApplication::testAttribute(Qt::AA_QtQuickUseDefaultSizePolicy); }
    void setUseDefaultSizePolicy(bool policy) { QCoreApplication::setAttribute(Qt::AA_QtQuickUseDefaultSizePolicy, policy); }

public slots:
    void resetBindingLoopDetectedFlag() { mBindingLoopDetected = false; }

    void qmlEngineAvailable(QQmlEngine *engine)
    {
        connect(engine, &QQmlEngine::warnings, this, &Setup::qmlWarnings);
        qmlRegisterSingletonInstance("org.qtproject.Test", 1, 0, "LayoutSetup", this);
    }

    void qmlWarnings(const QList<QQmlError> &warnings)
    {
        for (const auto &error : warnings) {
            if (error.messageType() == QtWarningMsg && error.description().contains(QStringLiteral("Binding loop detected")))
                mBindingLoopDetected = true;
        }
    }

private:
    bool mBindingLoopDetected = false;
};

QUICK_TEST_MAIN_WITH_SETUP(tst_qquicklayouts, Setup)

#include "tst_qquicklayouts.moc"
