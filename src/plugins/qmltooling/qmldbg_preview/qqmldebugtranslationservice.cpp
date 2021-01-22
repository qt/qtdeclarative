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

#include "qqmldebugtranslationservice.h"

QT_BEGIN_NAMESPACE

QQmlDebugTranslationServiceImpl::QQmlDebugTranslationServiceImpl(QObject *parent) :
    QQmlDebugTranslationService(1, parent)
{
}

void QQmlDebugTranslationServiceImpl::messageReceived(const QByteArray &message)
{
    Q_UNUSED(message)
}

QString QQmlDebugTranslationServiceImpl::foundElidedText(QObject *textObject, const QString &layoutText, const QString &elideText)
{
    Q_UNUSED(textObject)
    Q_UNUSED(layoutText)
    return elideText;
}

void QQmlDebugTranslationServiceImpl::foundTranslationBinding(QQmlTranslationBinding *binding, QObject *scopeObject, QQmlContextData *contextData)
{
    Q_UNUSED(binding)
    Q_UNUSED(scopeObject)
    Q_UNUSED(contextData)
}

QT_END_NAMESPACE
