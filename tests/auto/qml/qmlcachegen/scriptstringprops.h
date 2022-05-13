// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#ifndef SCRIPT_STRING_PROPS_H
#define SCRIPT_STRING_PROPS_H

#include <QObject>
#include <QQmlScriptString>
#include <qqml.h>

class ScriptStringProps  :public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QQmlScriptString undef READ undef WRITE setUndef NOTIFY undefChanged)
    Q_PROPERTY(QQmlScriptString nul READ nul WRITE setNul NOTIFY nulChanged)
    Q_PROPERTY(QQmlScriptString str READ str WRITE setStr NOTIFY strChanged)
    Q_PROPERTY(QQmlScriptString num READ num WRITE setNum NOTIFY numChanged)
    Q_PROPERTY(QQmlScriptString bol READ bol WRITE setBol NOTIFY bolChanged)

public:
    const QQmlScriptString &undef() const;
    void setUndef(const QQmlScriptString &newUndef);

    const QQmlScriptString &nul() const;
    void setNul(const QQmlScriptString &newNul);

    const QQmlScriptString &str() const;
    void setStr(const QQmlScriptString &newStr);

    const QQmlScriptString &num() const;
    void setNum(const QQmlScriptString &newNum);

    const QQmlScriptString &bol() const;
    void setBol(const QQmlScriptString &newBol);
signals:
    void undefChanged();

    void nulChanged();

    void strChanged();

    void numChanged();

    void bolChanged();

public:
    QQmlScriptString m_undef;
    QQmlScriptString m_nul;
    QQmlScriptString m_str;
    QQmlScriptString m_num;
    QQmlScriptString m_bol;
};

#endif
