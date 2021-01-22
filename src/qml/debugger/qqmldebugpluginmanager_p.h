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

#ifndef QQMLDEBUGPLUGINMANAGER_P_H
#define QQMLDEBUGPLUGINMANAGER_P_H

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

#include <QDebug>
#include <private/qtqmlglobal_p.h>
#include <private/qfactoryloader_p.h>

QT_BEGIN_NAMESPACE

#if !QT_CONFIG(qml_debug)

#define Q_QML_DEBUG_PLUGIN_LOADER(interfaceName)\
    interfaceName *load##interfaceName(const QString &key)\
    {\
        qWarning() << "Qml Debugger: QtQml is not configured for debugging. Ignoring request for"\
                   << "debug plugin" << key;\
        return 0;\
    }\
    QList<QJsonObject> metaDataFor##interfaceName()\
    {\
        return QList<QJsonObject>();\
    }
#define Q_QML_IMPORT_DEBUG_PLUGIN(className)

#else // QT_CONFIG(qml_debug)

#define Q_QML_DEBUG_PLUGIN_LOADER(interfaceName)\
    Q_GLOBAL_STATIC_WITH_ARGS(QFactoryLoader, interfaceName##Loader,\
        (interfaceName##Factory_iid, QLatin1String("/qmltooling")))\
    interfaceName *load##interfaceName(const QString &key)\
    {\
        return qLoadPlugin<interfaceName, interfaceName##Factory>(interfaceName##Loader(), key);\
    }\
    QList<QJsonObject> metaDataFor##interfaceName()\
    {\
        return interfaceName##Loader()->metaData();\
    }

#endif // QT_CONFIG(qml_debug)

QT_END_NAMESPACE
#endif // QQMLDEBUGPLUGINMANAGER_P_H
