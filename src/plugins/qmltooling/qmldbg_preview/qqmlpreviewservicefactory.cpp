// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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
