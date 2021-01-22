/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#ifndef QDEBUGMESSAGESERVICE_H
#define QDEBUGMESSAGESERVICE_H

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

#include <private/qqmldebugserviceinterfaces_p.h>

#include <QtCore/qlogging.h>
#include <QtCore/qmutex.h>
#include <QtCore/qelapsedtimer.h>

QT_BEGIN_NAMESPACE

class QQmlDebugTranslationServicePrivate;

class QQmlDebugTranslationServiceImpl : public QQmlDebugTranslationService
{
    Q_OBJECT
public:
    //needs to be in sync with QQmlDebugTranslationClient in qqmldebugtranslationclient_p.h
    enum Command {
        ChangeLanguage,
        ChangeWarningColor,
        ChangeElidedTextWarningString,
        SetDebugTranslationServiceLogFile,
        EnableElidedTextWarning,
        DisableElidedTextWarning,
        TestAllLanguages
    };
    QQmlDebugTranslationServiceImpl(QObject *parent = 0);

    QString foundElidedText(QObject *textObject, const QString &layoutText, const QString &elideText) override;
    void foundTranslationBinding(QQmlTranslationBinding *binding, QObject *scopeObject, QQmlContextData *contextData) override;
    void messageReceived(const QByteArray &message) override;
};

QT_END_NAMESPACE

#endif // QDEBUGMESSAGESERVICE_H
