// Copyright (C) 2016 Pelagicore AG
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQMLLOGGINGCATEGORY_P_H
#define QQMLLOGGINGCATEGORY_P_H

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

#include <QtCore/qobject.h>
#include <QtCore/qstring.h>
#include <QtCore/qloggingcategory.h>

#include <QtQml/qqmlparserstatus.h>
#include <QtQml/qqml.h>
#include <QtCore/private/qglobal_p.h>

#include <memory>

QT_BEGIN_NAMESPACE

class QQmlLoggingCategory : public QObject, public QQmlParserStatus
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)

    Q_PROPERTY(QString name READ name WRITE setName)
    Q_PROPERTY(DefaultLogLevel defaultLogLevel READ defaultLogLevel WRITE setDefaultLogLevel REVISION(2, 12))
    QML_NAMED_ELEMENT(LoggingCategory)
    QML_ADDED_IN_VERSION(2, 8)

public:
    enum DefaultLogLevel {
        Debug = QtDebugMsg,
        Info = QtInfoMsg,
        Warning = QtWarningMsg,
        Critical = QtCriticalMsg,
        Fatal = QtFatalMsg
    };
    Q_ENUM(DefaultLogLevel);

    QQmlLoggingCategory(QObject *parent = nullptr);
    virtual ~QQmlLoggingCategory();

    DefaultLogLevel defaultLogLevel() const;
    void setDefaultLogLevel(DefaultLogLevel defaultLogLevel);
    QString name() const;
    void setName(const QString &name);

    QLoggingCategory *category() const;

    void classBegin() override;
    void componentComplete() override;

private:
    QByteArray m_name;
    std::unique_ptr<QLoggingCategory> m_category;
    DefaultLogLevel m_defaultLogLevel = Debug;
    bool m_initialized;
};

QT_END_NAMESPACE

#endif // QQMLLOGGINGCATEGORY_H
