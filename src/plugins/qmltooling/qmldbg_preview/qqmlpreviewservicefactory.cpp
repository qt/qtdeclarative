/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QML preview debug service.
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
******************************************************************************/

#include "qqmlpreviewservicefactory.h"
#include "qqmlpreviewservice.h"
#if QT_CONFIG(translation)
#include "qqmldebugtranslationservice.h"
#endif

QT_BEGIN_NAMESPACE

QQmlDebugService *QQmlPreviewServiceFactory::create(const QString &key)
{
    if (key == QQmlPreviewServiceImpl::s_key)
        return new QQmlPreviewServiceImpl(this);
#if QT_CONFIG(translation)
    if (key == QQmlDebugTranslationServiceImpl::s_key)
        return new QQmlDebugTranslationServiceImpl(this);
#endif
    return nullptr;
}

QT_END_NAMESPACE

#include "moc_qqmlpreviewservicefactory.cpp"
