/****************************************************************************
**
** Copyright (C) 2016 Research In Motion.
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

#ifndef QQMLAPPLICATIONENGINE_P_H
#define QQMLAPPLICATIONENGINE_P_H

#include "qqmlapplicationengine.h"
#include "qqmlengine_p.h"
#include <QCoreApplication>
#include <QFileInfo>
#include <QLibraryInfo>

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

QT_BEGIN_NAMESPACE

class QFileSelector;
class Q_QML_PRIVATE_EXPORT QQmlApplicationEnginePrivate : public QQmlEnginePrivate
{
    Q_DECLARE_PUBLIC(QQmlApplicationEngine)
public:
    QQmlApplicationEnginePrivate(QQmlEngine *e);
    ~QQmlApplicationEnginePrivate();
    void init();
    void cleanUp();

    void startLoad(const QUrl &url, const QByteArray &data = QByteArray(), bool dataFlag = false);
    void _q_loadTranslations();
    void finishLoad(QQmlComponent *component);
    QList<QObject *> objects;
    QVariantMap initialProperties;
    QString translationsDirectory;
#if QT_CONFIG(translation)
    QScopedPointer<QTranslator> activeTranslator;
#endif
};

QT_END_NAMESPACE

#endif
