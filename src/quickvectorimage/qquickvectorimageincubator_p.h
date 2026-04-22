// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQUICKVECTORIMAGEINCUBATOR_P_H
#define QQUICKVECTORIMAGEINCUBATOR_P_H

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

#include <QtCore/qthread.h>
#include <QtQml/qqmlincubator.h>

#include <QtQuickVectorImage/qtquickvectorimageexports.h>
#include <QtQuickVectorImageGenerator/private/qquickitemgenerator_p.h>
#include <QtQuickVectorImageGenerator/private/qquickvectorimageplugin_p.h>
#include <QtQuickVectorImageGenerator/private/qquickvectorimageglobal_p.h>

QT_BEGIN_NAMESPACE

class QQuickVectorImagePluginGenerator;
class QQmlContext;

class QQuickVectorImageWorker : public QObject
{
    Q_OBJECT
public:
    QQuickVectorImageWorker() = default;

    void createGenerator(const QString &fileName,
                         QQuickVectorImageGenerator::GeneratorFlags flags)
    {
        m_generator.reset(new QQuickItemGenerator(fileName, flags));
    }

    QQuickItemGenerator *generator() const
    {
        return m_generator.get();
    }

    void addPluginGenerator(QQuickVectorImagePluginGenerator *pluginGenerator)
    {
        m_pluginGenerators.push_back(std::unique_ptr<QQuickVectorImagePluginGenerator>(pluginGenerator));
    }

public Q_SLOTS:
    void process();

Q_SIGNALS:
    void finished();

private:
    std::unique_ptr<QQuickItemGenerator> m_generator;
    std::vector<std::unique_ptr<QQuickVectorImagePluginGenerator> > m_pluginGenerators;
};

class Q_QUICKVECTORIMAGE_EXPORT QQuickVectorImageIncubator : public QObject, public QQmlIncubator
{
    Q_OBJECT
public:
    QQuickVectorImageIncubator(IncubationMode incubationMode,
                               QQmlContext *context,
                               QObject *parent = nullptr);
    ~QQuickVectorImageIncubator();

    void start(const QString &fileName,
               QQuickVectorImageGenerator::GeneratorFlags flags);

    QQmlIncubator::Status status() const;

protected:
    void statusChanged(Status status) override;

private Q_SLOTS:
    void generatorFinished();
    void componentUpdated();

Q_SIGNALS:
    void statusUpdated();

private:
    std::unique_ptr<QQmlComponent> m_component;
    std::unique_ptr<QQuickVectorImageWorker> m_generatorWorker;
    std::unique_ptr<QThread> m_workerThread;
    QQmlContext *m_qmlContext = nullptr;
    QQmlIncubator::Status m_status = QQmlIncubator::Null;
};

QT_END_NAMESPACE

#endif // QQUICKVECTORIMAGEINCUBATOR_P_H
