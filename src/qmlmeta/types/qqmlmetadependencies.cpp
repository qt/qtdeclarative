// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#include "qqmlmetadependencies_p.h"
#include <private/qtqmlglobal_p.h>
#include <QtQml/qqmlextensionplugin.h>

QT_BEGIN_NAMESPACE

// Prevent the linker from removing those dependencies, so that we don't have to load
// them from plugins.

void qml_register_types_QtQml_Models();
Q_GHS_KEEP_REFERENCE(qml_register_types_QtQml_Models);
#if QT_CONFIG(qml_worker_script)
void qml_register_types_QtQml_WorkerScript();
Q_GHS_KEEP_REFERENCE(qml_register_types_QtQml_WorkerScript);
#endif

bool QQmlMetaDependencies::collect() {
    volatile auto models = &qml_register_types_QtQml_Models;
#if QT_CONFIG(qml_worker_script)
    volatile auto workerScript = &qml_register_types_QtQml_WorkerScript;
    return models && workerScript;
#else
    return models;
#endif
}

QT_END_NAMESPACE
