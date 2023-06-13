// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtQml/qqmlengine.h>
#include <QtQuickTest/quicktest.h>

using namespace Qt::Literals;

class Setup : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool shortcutsSupported READ areShortcutsSupported CONSTANT FINAL)

    Q_PROPERTY(int shortcutInt MEMBER m_shortcutInt CONSTANT FINAL)
    Q_PROPERTY(QString shortcutString MEMBER m_shortcutString CONSTANT FINAL)
    Q_PROPERTY(QKeySequence shortcutKeySequence MEMBER m_shortcutKeySequence CONSTANT FINAL)

    const int m_shortcutInt = QKeySequence::Print;
    const QString m_shortcutString = u"CTRL+P"_s;
    const QKeySequence m_shortcutKeySequence{ Qt::CTRL | Qt::Key_P };

public:
    bool areShortcutsSupported() const
    {
#if QT_CONFIG(shortcut)
        return true;
#else
        return false;
#endif
    }

public slots:
    void qmlEngineAvailable(QQmlEngine *)
    {
        qmlRegisterSingletonInstance("org.qtproject.Test", 1, 0, "TestHelper", this);
    }
};

QUICK_TEST_MAIN_WITH_SETUP(tst_platform, Setup)

#include "tst_platform.moc"
