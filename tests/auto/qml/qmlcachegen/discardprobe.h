// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
#ifndef DISCARD_PROBE_H
#define DISCARD_PROBE_H

#include <QObject>
#include <QQmlScriptString>
#include <qqml.h>

class DiscardProbe : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QQmlScriptString lit READ lit WRITE setLit NOTIFY litChanged)
    Q_PROPERTY(QQmlScriptString expr READ expr WRITE setExpr NOTIFY exprChanged)

public:
    QQmlScriptString lit() const { return m_lit; }
    void setLit(const QQmlScriptString &lit)
    {
        if (lit == m_lit)
            return;
        m_lit = lit;
        emit litChanged();
    }

    QQmlScriptString expr() const { return m_expr; }
    void setExpr(const QQmlScriptString &expr)
    {
        if (expr == m_expr)
            return;
        m_expr = expr;
        emit exprChanged();
    }

signals:
    void litChanged();
    void exprChanged();

public:
    QQmlScriptString m_lit;
    QQmlScriptString m_expr;
};

#endif
