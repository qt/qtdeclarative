/****************************************************************************
**
** Copyright (C) 2016 Pelagicore AG
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

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

QT_BEGIN_NAMESPACE

class QQmlLoggingCategory : public QObject, public QQmlParserStatus
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)

    Q_PROPERTY(QString name READ name WRITE setName)
    Q_PROPERTY(DefaultLogLevel defaultLogLevel READ defaultLogLevel WRITE setDefaultLogLevel REVISION 12)
    QML_NAMED_ELEMENT(LoggingCategory)
    QML_ADDED_IN_MINOR_VERSION(8)

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
    QScopedPointer<QLoggingCategory> m_category;
    DefaultLogLevel m_defaultLogLevel = Debug;
    bool m_initialized;
};

QT_END_NAMESPACE

#endif // QQMLLOGGINGCATEGORY_H
