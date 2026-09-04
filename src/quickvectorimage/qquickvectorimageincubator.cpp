// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickvectorimageincubator_p.h"
#include "qquickvectorimageincubator_p_p.h"
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
    : QObject(*new QQuickVectorImageIncubatorPrivate, parent)
    , QQmlIncubator(incubationMode)
{
    Q_D(QQuickVectorImageIncubator);
    d->qmlContext = context;
}

QQuickVectorImageIncubator::~QQuickVectorImageIncubator()
{
    Q_D(QQuickVectorImageIncubator);
    if (d->workerThread != nullptr && d->workerThread->isRunning()) {
        d->workerThread->quit();
        d->workerThread->wait();
    }
}

QQmlIncubator::Status QQuickVectorImageIncubator::status() const
{
    Q_D(const QQuickVectorImageIncubator);
    return d->status;
}

void QQuickVectorImageIncubator::start(const QString &fileName,
                                       QQuickVectorImageGenerator::GeneratorFlags flags)
{
    Q_D(QQuickVectorImageIncubator);
    Q_ASSERT(d->generatorWorker == nullptr);
    Q_ASSERT(d->workerThread == nullptr);

    d->status = Loading;
    emit statusUpdated();

    const bool asynchronous = flags.testFlag(QQuickVectorImageGenerator::AsynchronousLoading);
    if (asynchronous)
        d->workerThread.reset(new QThread);

    d->generatorWorker.reset(new QQuickVectorImageWorker);
    d->generatorWorker->createGenerator(fileName, flags);
    connect(d->generatorWorker.get(), &QQuickVectorImageWorker::finished,
            this, &QQuickVectorImageIncubator::generatorFinished);

    if (flags.testFlag(QQuickVectorImageGenerator::AssumeTrustedSource)) {
        QFactoryLoader *loader = vectorImagePluginLoader();

        const qsizetype count = loader->keyMap().size();
        for (qsizetype i = 0; i < count; ++i) {
            QQuickVectorImagePlugin *plugin = qobject_cast<QQuickVectorImagePlugin *>(loader->instance(i));
            if (plugin != nullptr) {
                QQuickVectorImagePluginGenerator *pluginGenerator = plugin->createGenerator(fileName);
                if (pluginGenerator != nullptr)
                    d->generatorWorker->addPluginGenerator(pluginGenerator);
            }
        }
    }

    if (d->workerThread != nullptr) {
        d->generatorWorker->moveToThread(d->workerThread.get());
        d->workerThread->start();
    }

    // Trigger generating
    QMetaObject::invokeMethod(d->generatorWorker.get(), &QQuickVectorImageWorker::process, Qt::AutoConnection);
}

void QQuickVectorImageIncubator::generatorFinished()
{
    Q_D(QQuickVectorImageIncubator);
    Q_ASSERT(d->component == nullptr);
    Q_ASSERT(d->generatorWorker != nullptr);
    Q_ASSERT(d->generatorWorker->generator() != nullptr);
    Q_ASSERT(d->workerThread == nullptr || d->workerThread->isRunning());

    const QQuickVectorImageGenerator::ErrorState errorState = d->generatorWorker->generator()->errorState();
    const bool asynchronous = d->generatorWorker->generator()->generatorFlags().testFlag(QQuickVectorImageGenerator::AsynchronousLoading);
    const QByteArray result = d->generatorWorker->generator()->result();

    d->generatorWorker->disconnect(this);
    d->generatorWorker.release()->deleteLater();

    if (d->workerThread != nullptr) {
        d->workerThread->quit();
        d->workerThread->wait();
        d->workerThread.reset(nullptr);
    }

    if (Q_LIKELY(errorState == QQuickVectorImageGenerator::NoError)) {
        QQmlEngine *engine = d->qmlContext->engine();
        if (Q_UNLIKELY(engine == nullptr)) {
            qCWarning(lcQuickVectorImage) << "QQuickVectorImageIncubator::generatorFinished: Requires QML engine";
            return;
        }

        d->component.reset(new QQmlComponent(engine));
        connect(d->component.get(), &QQmlComponent::statusChanged,
                this, &QQuickVectorImageIncubator::componentUpdated);
        if (asynchronous) {
            QQmlComponentPrivate *cd = QQmlComponentPrivate::get(d->component.get());
            cd->setData(result, QUrl{}, QQmlComponent::Asynchronous);
            emit d->component->statusChanged(d->component->status());
        } else {
            d->component->setData(result, QUrl{});
        }
    } else {
        d->status = Error;
        emit statusUpdated();
    }
}

void QQuickVectorImageIncubator::componentUpdated()
{
    Q_D(QQuickVectorImageIncubator);
    Q_ASSERT(d->component != nullptr);
    Q_ASSERT(d->generatorWorker == nullptr);
    Q_ASSERT(d->workerThread == nullptr);

    if (!d->component->isLoading()) {
        if (d->component->isReady()) {
            d->component->create(*this, d->qmlContext);
        } else {
            qCWarning(lcQuickVectorImage) << "Component failed to load:"
                                          << d->component->errorString();
            d->status = Error;
            emit statusUpdated();
        }

        d->component.release()->deleteLater();
    }
}

void QQuickVectorImageIncubator::statusChanged(Status status)
{
    Q_D(QQuickVectorImageIncubator);
    if (d->status == status)
        return;
    d->status = status;
    emit statusUpdated();
}

QT_END_NAMESPACE
