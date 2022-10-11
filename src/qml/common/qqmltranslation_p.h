// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQMLTRANSLATION_P_H
#define QQMLTRANSLATION_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/qstring.h>

#include <private/qv4qmlcontext_p.h>

QT_BEGIN_NAMESPACE

class Q_QML_PRIVATE_EXPORT QQmlTranslation
{
public:
    class Q_QML_PRIVATE_EXPORT QsTrData
    {
        QByteArray context;
        QByteArray text;
        QByteArray comment;
        int number;

    public:
        QsTrData(const QString &fileNameForContext, const QString &text, const QString &comment,
                 int number);
        QString translate() const;
        QString serializeForQmltc() const;
        QString idForQmlDebug() const;
    };

    class Q_QML_PRIVATE_EXPORT QsTrIdData
    {
        QByteArray id;
        int number;

    public:
        QsTrIdData(const QString &id, int number);
        QString translate() const;
        QString serializeForQmltc() const;
        QString idForQmlDebug() const;
    };

    // The static analyzer hates std::monostate in std::variant because
    // that results in various uninitialized memory "problems". Just use
    // std::nullptr_t to indicate "empty".
    using Data = std::variant<std::nullptr_t, QsTrData, QsTrIdData>;

private:
    Data data;

public:
    QQmlTranslation(const Data &d);
    QQmlTranslation();
    QString translate() const;
    QString serializeForQmltc() const;
    QString idForQmlDebug() const;

    static QString contextFromQmlFilename(const QString &qmlFilename);
};

QT_END_NAMESPACE

#endif // QQMLTRANSLATION_P_H
