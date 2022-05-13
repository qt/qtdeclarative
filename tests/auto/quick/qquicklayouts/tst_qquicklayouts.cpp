// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlcontext.h>
#include <QtQuickTest/quicktest.h>

class Setup : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool bindingLoopDetected READ wasBindingLoopDetected FINAL)

public:
    Setup() {}

    bool wasBindingLoopDetected() const { return mBindingLoopDetected; }

public slots:
    void reset() { mBindingLoopDetected = false; }

    void qmlEngineAvailable(QQmlEngine *engine)
    {
        connect(engine, &QQmlEngine::warnings, this, &Setup::qmlWarnings);

        qmlRegisterSingletonInstance("org.qtproject.Test", 1, 0, "BindingLoopDetector", this);
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
