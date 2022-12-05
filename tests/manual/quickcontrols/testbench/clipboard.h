// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef CLIPBOARD_H
#define CLIPBOARD_H

#include <QObject>
#include <QJSValue>
#include <QVariant>

class Clipboard : public QObject
{
    Q_OBJECT
public:
    explicit Clipboard(QObject *parent = nullptr);

public slots:
    void copy(const QJSValue &keyValueMap);
    QVariant paste() const;

//    void copyPaletteSettingsToClipboard();
//    void importPaletteSettingsFromClipboard();
};

#endif // CLIPBOARD_H
