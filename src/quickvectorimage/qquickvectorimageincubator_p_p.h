// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQUICKVECTORIMAGEINCUBATOR_P_P_H
#define QQUICKVECTORIMAGEINCUBATOR_P_P_H

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

#include "qquickvectorimageincubator_p.h"
#include <QtCore/private/qobject_p.h>
#include <QtQml/private/qqmlrefcount_p.h>

#include <memory>

QT_BEGIN_NAMESPACE

class QQuickVectorImageIncubatorPrivate: public QObjectPrivate
{
public:
    class QmlComponentGuard {
    public:
        QmlComponentGuard() = default;
        QmlComponentGuard(QmlComponentGuard &&) = default;
        ~QmlComponentGuard();

        QmlComponentGuard &operator=(QmlComponentGuard &&) = delete;

        bool isNull() const
        {
            return m_component == nullptr && m_baseCompilationUnit == nullptr;
        }

        QQmlComponent *takeComponent();

        QQmlComponent *component() const
        {
            return m_component.get();
        }

        void setComponent(QQmlComponent *component)
        {
            Q_ASSERT(m_component == nullptr);
            m_component.reset(component);
        }

    private:
        std::unique_ptr<QQmlComponent> m_component;
        QQmlRefPointer<QV4::CompiledData::CompilationUnit> m_baseCompilationUnit;
    };

    static QQuickVectorImageIncubatorPrivate *get(QQuickVectorImageIncubator *incubator)
    {
        return static_cast<QQuickVectorImageIncubatorPrivate *>(QObjectPrivate::get(incubator));
    }

    QmlComponentGuard takeComponentGuard();

    std::unique_ptr<QQuickVectorImageWorker> generatorWorker;
    std::unique_ptr<QThread> workerThread;
    QQmlContext *qmlContext = nullptr;
    QQmlIncubator::Status status = QQmlIncubator::Null;
    QmlComponentGuard componentGuard;
};

QT_END_NAMESPACE

#endif // QQUICKVECTORIMAGEINCUBATOR_P_P_H
