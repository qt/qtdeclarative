// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQUICKVECTORIMAGEPLUGIN_P_H
#define QQUICKVECTORIMAGEPLUGIN_P_H

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

#include <QObject>
#include <QtQuickVectorImageGenerator/private/qquickgenerator_p.h>

#define QQuickVectorImageFormatsPluginFactory_iid "org.qt-project.Qt.QVectorImageFormatsPluginFactory"

QT_BEGIN_NAMESPACE

class Q_QUICKVECTORIMAGEGENERATOR_EXPORT QQuickVectorImagePluginGenerator
{
public:
    virtual ~QQuickVectorImagePluginGenerator();
    virtual bool generate(const QString &fileName, QQuickGenerator *generator) = 0;
};

class Q_QUICKVECTORIMAGEGENERATOR_EXPORT QQuickVectorImagePlugin
{
public:
    QQuickVectorImagePlugin();
    virtual ~QQuickVectorImagePlugin();

    virtual QQuickVectorImagePluginGenerator *createGenerator(const QString &fileName) = 0;
};

#define QQuickVectorImageFormatsPluginInterface_iid "org.qt-project.Qt.QVectorImageFormatsPluginInterface"
Q_DECLARE_INTERFACE(QQuickVectorImagePlugin, QQuickVectorImageFormatsPluginInterface_iid)

QT_END_NAMESPACE

#endif // QQUICKVECTORIMAGEPLUGIN_P_H
