// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickvectorimageincubator_p.h"
#include <QtCore/private/qfactoryloader_p.h>
#include <QtQml/qqmlcontext.h>
#include <QtQml/private/qqmlcomponent_p.h>

QT_BEGIN_NAMESPACE

Q_GLOBAL_STATIC_WITH_ARGS(QFactoryLoader, vectorImagePluginLoader,
                          (QQuickVectorImageFormatsPluginFactory_iid,
                           QLatin1String("/vectorimageformats"),
                           Qt::CaseInsensitive))

void QQuickVectorImageWorker::process()
{
    Q_ASSERT(m_generator != nullptr);

    // If we assume trusted source, we try plugins first
    bool generatedWithPlugin = false;
    for (const auto &pluginGenerator : std::as_const(m_pluginGenerators)) {
        if ((generatedWithPlugin = pluginGenerator->generate(m_generator->fileName(), m_generator.get())))
            break;
    }

    if (!generatedWithPlugin)
        m_generator->generate();

    emit finished();
}

QQuickVectorImageIncubator::QQuickVectorImageIncubator(IncubationMode incubationMode,
                                                       QQmlContext *context,
                                                       QObject *parent)
    : QObject(parent)
    , QQmlIncubator(incubationMode)
    , m_qmlContext(context)
{
}

QQuickVectorImageIncubator::~QQuickVectorImageIncubator()
{
    if (m_workerThread != nullptr && m_workerThread->isRunning()) {
        m_workerThread->quit();
        m_workerThread->wait();
    }
}

QQmlIncubator::Status QQuickVectorImageIncubator::status() const
{
    return m_status;
}

void QQuickVectorImageIncubator::start(const QString &fileName,
                                       QQuickVectorImageGenerator::GeneratorFlags flags)
{
    Q_ASSERT(m_generatorWorker == nullptr);
    Q_ASSERT(m_workerThread == nullptr);

    m_status = Loading;
    emit statusUpdated();

    const bool asynchronous = flags.testFlag(QQuickVectorImageGenerator::AsynchronousLoading);
    if (asynchronous)
        m_workerThread.reset(new QThread);

    m_generatorWorker.reset(new QQuickVectorImageWorker);
    m_generatorWorker->createGenerator(fileName, flags);
    connect(m_generatorWorker.get(), &QQuickVectorImageWorker::finished,
            this, &QQuickVectorImageIncubator::generatorFinished);

    if (flags.testFlag(QQuickVectorImageGenerator::AssumeTrustedSource)) {
        QFactoryLoader *loader = vectorImagePluginLoader();

        const qsizetype count = loader->keyMap().size();
        for (qsizetype i = 0; i < count; ++i) {
            QQuickVectorImagePlugin *plugin = qobject_cast<QQuickVectorImagePlugin *>(loader->instance(i));
            if (plugin != nullptr) {
                QQuickVectorImagePluginGenerator *pluginGenerator = plugin->createGenerator(fileName);
                if (pluginGenerator != nullptr)
                    m_generatorWorker->addPluginGenerator(pluginGenerator);
            }
        }
    }

    if (m_workerThread != nullptr) {
        m_generatorWorker->moveToThread(m_workerThread.get());
        m_workerThread->start();
    }

    // Trigger generating
    QMetaObject::invokeMethod(m_generatorWorker.get(), &QQuickVectorImageWorker::process, Qt::AutoConnection);
}

void QQuickVectorImageIncubator::generatorFinished()
{
    Q_ASSERT(m_component == nullptr);
    Q_ASSERT(m_generatorWorker != nullptr);
    Q_ASSERT(m_generatorWorker->generator() != nullptr);
    Q_ASSERT(m_workerThread == nullptr || m_workerThread->isRunning());

    const QQuickVectorImageGenerator::ErrorState errorState = m_generatorWorker->generator()->errorState();
    const bool asynchronous = m_generatorWorker->generator()->generatorFlags().testFlag(QQuickVectorImageGenerator::AsynchronousLoading);
    const QByteArray result = m_generatorWorker->generator()->result();
    const QString fileName = m_generatorWorker->generator()->fileName();

    m_generatorWorker->disconnect(this);
    m_generatorWorker.release()->deleteLater();

    if (m_workerThread != nullptr) {
        m_workerThread->quit();
        m_workerThread->wait();
        m_workerThread.reset(nullptr);
    }

    if (Q_LIKELY(errorState == QQuickVectorImageGenerator::NoError)) {
        QQmlEngine *engine = m_qmlContext->engine();
        if (Q_UNLIKELY(engine == nullptr)) {
            qCWarning(lcQuickVectorImage) << "QQuickVectorImageIncubator::generatorFinished: Requires QML engine";
            return;
        }

        m_component.reset(new QQmlComponent(engine));
        connect(m_component.get(), &QQmlComponent::statusChanged,
                this, &QQuickVectorImageIncubator::componentUpdated);
        if (asynchronous) {
            QQmlComponentPrivate *d = QQmlComponentPrivate::get(m_component.get());
            d->setData(result, QUrl{}, QQmlComponent::Asynchronous);
            emit m_component->statusChanged(m_component->status());
        } else {
            m_component->setData(result, QUrl{});
        }
    } else {
        m_status = Error;
        emit statusUpdated();
    }
}

void QQuickVectorImageIncubator::componentUpdated()
{
    Q_ASSERT(m_component != nullptr);
    Q_ASSERT(m_generatorWorker == nullptr);
    Q_ASSERT(m_workerThread == nullptr);

    if (!m_component->isLoading()) {
        if (m_component->isReady()) {
            m_component->create(*this, m_qmlContext);
        } else {
            qCWarning(lcQuickVectorImage) << "Component failed to load:"
                                          << m_component->errorString();
            m_status = Error;
            emit statusUpdated();
        }
        m_component.release()->deleteLater();
    }
}

void QQuickVectorImageIncubator::statusChanged(Status status)
{
    if (m_status == status)
        return;
    m_status = status;
    emit statusUpdated();
}

QT_END_NAMESPACE
