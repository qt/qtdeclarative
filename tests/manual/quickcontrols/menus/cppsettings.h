// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef CPPSETTINGS_H
#define CPPSETTINGS_H

#include <QObject>
#include <QQmlEngine>
#include <QSettings>

class CppSettings : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool dontUseNativeMenuWindows READ dontUseNativeMenuWindows WRITE setDontUseNativeMenuWindows
        NOTIFY dontUseNativeMenuWindowsChanged FINAL)
    Q_PROPERTY(bool dontUseNativeMenuBar READ dontUseNativeMenuBar WRITE setDontUseNativeMenuBar
        NOTIFY dontUseNativeMenuBarChanged FINAL)
    QML_ELEMENT
    QML_SINGLETON

public:
    explicit CppSettings(QObject *parent = nullptr);

    bool dontUseNativeMenuWindows() const;
    void setDontUseNativeMenuWindows(bool dontUseNativeMenuWindows);

    bool dontUseNativeMenuBar() const;
    void setDontUseNativeMenuBar(bool dontUseNativeMenuBar);

signals:
    void dontUseNativeMenuWindowsChanged();
    void dontUseNativeMenuBarChanged();

private:
    QSettings mSettings;
};

#endif // CPPSETTINGS_H
