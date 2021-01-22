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

#include "qqmldebugtranslationclient_p.h"
#include "qqmldebugconnection_p.h"

#include <QUrl>
#include <QDataStream>

#include <QDebug>
#include <QtPacketProtocol/private/qpacket_p.h>

QT_BEGIN_NAMESPACE

/*!
  \class QQmlDebugTranslationClient
  \internal

  \brief Client for the debug translation service

  The QQmlDebugTranslationClient can test if translated texts will fit.
 */

QQmlDebugTranslationClient::QQmlDebugTranslationClient(QQmlDebugConnection *client)
    : QQmlDebugClient(QLatin1String("DebugTranslation"), client)
{
}

void QQmlDebugTranslationClient::messageReceived(const QByteArray &data)
{
    Q_UNUSED(data);
}

void QQmlDebugTranslationClient::triggerLanguage(const QUrl &url, const QString &locale)
{
    Q_UNUSED(url)
    Q_UNUSED(locale)
}

QT_END_NAMESPACE
