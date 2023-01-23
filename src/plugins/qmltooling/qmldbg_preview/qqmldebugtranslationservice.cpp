// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqmldebugtranslationservice.h"
#include "proxytranslator.h"
#include "qqmlpreviewservice.h"

#include <QtCore/qtranslator.h>
#include <QtCore/qdebug.h>
#include <QtCore/qlibraryinfo.h>
#include <QtCore/qdir.h>
#include <QtCore/qfile.h>
#include <QtCore/qtimer.h>
#include <QtCore/qhash.h>
#include <QtCore/qpointer.h>

#include <private/qqmldebugtranslationprotocol_p.h>
#include <private/qqmldebugconnector_p.h>
#include <private/qversionedpacket_p.h>

#include <private/qqmlbinding_p.h>
#include <private/qqmlbinding_p.h>
#include <private/qquickstategroup_p.h>
#include <private/qquickitem_p.h>
#include <private/qquicktext_p.h>
#include <private/qdebug_p.h>

#include <QtQuick/qquickitem.h>

#include <qquickview.h>

QT_BEGIN_NAMESPACE

using namespace QQmlDebugTranslation;

QDebug operator<<(QDebug debug, const TranslationBindingInformation &translationBindingInformation)
{
    QQmlError error;
    error.setUrl(translationBindingInformation.compilationUnit->url());
    error.setLine(translationBindingInformation.line);
    error.setColumn(translationBindingInformation.column);
    error.setDescription(
        QString(QLatin1String(
            "QDebug translation binding"
        )));
    return debug << qPrintable(error.toString());
}

class QQmlDebugTranslationServicePrivate : public QObject
{
    Q_OBJECT
public:
    QQmlDebugTranslationServicePrivate(QQmlDebugTranslationServiceImpl *parent)
        : q(parent)
        , proxyTranslator(new ProxyTranslator)
    {
        connect(&translatableTextOccurrenceTimer, &QTimer::timeout,
            this, &QQmlDebugTranslationServicePrivate::sendTranslatableTextOccurrences);
    }

    void setState(const QString &stateName)
    {
        if (QQuickItem *rootItem = currentRootItem()) {
            QQuickStateGroup *stateGroup = QQuickItemPrivate::get(rootItem)->_states();
            if (stateGroup->findState(stateName)) {
                connect(stateGroup, &QQuickStateGroup::stateChanged,
                        this, &QQmlDebugTranslationServicePrivate::sendStateChanged,
                        Qt::ConnectionType(Qt::QueuedConnection | Qt::UniqueConnection));
                stateGroup->setState(stateName);
            }
            else
                qWarning() << "Could not switch the state" << stateName << "at" << rootItem;
        }
    }

    void sendStateChanged()
    {
        if (QQuickStateGroup *stateGroup = qobject_cast<QQuickStateGroup*>(sender()))
            currentStateName = stateGroup->state();
        QVersionedPacket<QQmlDebugConnector> packet;
        packet << Reply::StateChanged << currentStateName;
        emit q->messageToClient(q->name(), packet.data());
    }

    void sendStateList()
    {
        QVersionedPacket<QQmlDebugConnector> packet;
        packet << Reply::StateList;
        QVector<QmlState> qmlStates;

        if (QQuickItem *rootItem = currentRootItem()) {
            QQuickStateGroup *stateGroup = QQuickItemPrivate::get(rootItem)->_states();

            QList<QQuickState *> states = stateGroup->states();

            for (QQuickState *state : states) {
                QmlState qmlState;
                qmlState.name = state->name();
                qmlStates.append(qmlState);
            }
        }

        packet << qmlStates;
        emit q->messageToClient(q->name(), packet.data());
    }

    void setWatchTextElides(bool s)
    {
        // TODO: for disabling we need to keep track which one were enabled
        if (s == false)
            qWarning() << "disable WatchTextElides is not implemented";
        watchTextElides = s;
        for (auto &&information : std::as_const(objectTranslationBindingMultiMap)) {
            QObject *scopeObject = information.scopeObject;
            int elideIndex = scopeObject->metaObject()->indexOfProperty("elide");
            if (elideIndex >= 0) {
                auto elideProperty = scopeObject->metaObject()->property(elideIndex);
                elideProperty.write(scopeObject, Qt::ElideRight);
            }
        }
    }

    QString getStyleNameForFont(const QFont& font)
    {
        if (font.styleName() != "")
            return font.styleName();
        QString styleName;
        if (font.bold())
            styleName.append("Bold ");
        if (font.italic())
            styleName.append("Italic " );
        if (font.strikeOut())
            styleName.append("StrikeThrough ");
        if (font.underline())
            styleName.append("Underline ");
        return styleName.trimmed();
    }

    void sendTranslatableTextOccurrences()
    {

        QVersionedPacket<QQmlDebugConnector> packet;
        packet << Reply::TranslatableTextOccurrences;

        QVector<QmlElement> qmlElements;

        for (auto &&information : std::as_const(objectTranslationBindingMultiMap)) {

            QObject *scopeObject = information.scopeObject;
            auto compilationUnit = information.compilationUnit;
            auto metaObject = scopeObject->metaObject();

            int textIndex = metaObject->indexOfProperty(information.propertyName.toLatin1());
            if (textIndex >= 0) {

                QmlElement qmlElement;

                qmlElement.codeMarker = codeMarker(information);

                auto textProperty = scopeObject->metaObject()->property(textIndex);
                qmlElement.propertyName = textProperty.name();
                qmlElement.translationId = information.translation.idForQmlDebug();
                qmlElement.translatedText = textProperty.read(scopeObject).toString();
                qmlElement.elementId = qmlContext(scopeObject)->nameForObject(scopeObject);

                QFont font = scopeObject->property("font").value<QFont>();
                qmlElement.fontFamily = font.family();
                qmlElement.fontPointSize = font.pointSize();
                qmlElement.fontPixelSize = font.pixelSize();
                qmlElement.fontStyleName = getStyleNameForFont(font);
                qmlElement.horizontalAlignment =
                        scopeObject->property("horizontalAlignment").toInt();
                qmlElement.verticalAlignment = scopeObject->property("verticalAlignment").toInt();

                QQmlType qmlType = QQmlMetaType::qmlType(metaObject);
                qmlElement.elementType = qmlType.qmlTypeName() + "/" + qmlType.typeName();
                qmlElement.stateName = currentStateName;
                qmlElements.append(qmlElement);

            } else {
                QString warningMessage = "(QQmlDebugTranslationService can not resolve %1 - %2:"\
                                         " this should never happen)";
                const QString id = qmlContext(scopeObject)->nameForObject(scopeObject);
                qWarning().noquote() << warningMessage.arg(id, information.propertyName);
            }
        }
        std::sort(qmlElements.begin(), qmlElements.end(), [](const auto &l1, const auto &l2){
            return l1.codeMarker < l2.codeMarker;
        });

        packet << qmlElements;
        emit q->messageToClient(q->name(), packet.data());
    }

    void sendLanguageChanged()
    {
        QVersionedPacket<QQmlDebugConnector> packet;
        packet << Reply::LanguageChanged;
        emit q->messageToClient(q->name(), packet.data());
    }

    void sendTranslationIssues()
    {
        QVersionedPacket<QQmlDebugConnector> packet;
        packet << Reply::TranslationIssues;

        QVector<TranslationIssue> issues;
        for (auto &&information : std::as_const(objectTranslationBindingMultiMap)) {
            if (!proxyTranslator->hasTranslation(information)) {
                TranslationIssue issue;
                issue.type = TranslationIssue::Type::Missing;
                issue.codeMarker = codeMarker(information);
                issue.language = proxyTranslator->currentUILanguages();
                issues.append(issue);
            }

            QObject *scopeObject = information.scopeObject;
            QQuickText *quickText = static_cast<QQuickText*>(scopeObject);
            if (quickText) {
                if (quickText->truncated()) {
                    TranslationIssue issue;
                    issue.type = TranslationIssue::Type::Elided;
                    issue.codeMarker = codeMarker(information);
                    issue.language = proxyTranslator->currentUILanguages();
                    issues.append(issue);
                }
            }
        }
        std::sort(issues.begin(), issues.end(), [](const auto &l1, const auto &l2){
            return l1.codeMarker < l2.codeMarker;
        });
        packet << issues;
        emit q->messageToClient(q->name(), packet.data());
    }

    QQmlDebugTranslationServiceImpl *q;

    bool watchTextElides = false;
    QMultiMap<QObject*, TranslationBindingInformation> objectTranslationBindingMultiMap;
    QHash<QObject*, QVector<QMetaObject::Connection>> elideConnections;
    ProxyTranslator *proxyTranslator;

    bool enableWatchTranslations = false;
    QTimer translatableTextOccurrenceTimer;
    QList<QPointer<QQuickItem>> translatableTextOccurrences;

    QQuickItem *currentRootItem()
    {
        if (QQmlPreviewServiceImpl *service = QQmlDebugConnector::service<QQmlPreviewServiceImpl>())
            return service->currentRootItem();
        if (currentQuickView)
            return currentQuickView->rootObject();
        return nullptr;
    }
    QQuickView* currentQuickView = nullptr;

private:
    CodeMarker codeMarker(const TranslationBindingInformation &information)
    {
        CodeMarker c;
        c.url = information.compilationUnit->url();
        c.line = information.line;
        c.column = information.column;
        return c;
    }
    QString currentStateName;
};

QQmlDebugTranslationServiceImpl::QQmlDebugTranslationServiceImpl(QObject *parent)
    : QQmlDebugTranslationService(1, parent)
{
    d = new QQmlDebugTranslationServicePrivate(this);

    connect(this, &QQmlDebugTranslationServiceImpl::watchTextElides,
            d, &QQmlDebugTranslationServicePrivate::setWatchTextElides,
            Qt::QueuedConnection);

    connect(this, &QQmlDebugTranslationServiceImpl::language,
            d->proxyTranslator, &ProxyTranslator::setLanguage,
            Qt::QueuedConnection);

    connect(this, &QQmlDebugTranslationServiceImpl::state,
            d, &QQmlDebugTranslationServicePrivate::setState,
            Qt::QueuedConnection);

    connect(this, &QQmlDebugTranslationServiceImpl::stateList,
            d, &QQmlDebugTranslationServicePrivate::sendStateList,
            Qt::QueuedConnection);

    connect(d->proxyTranslator, &ProxyTranslator::languageChanged,
            d, &QQmlDebugTranslationServicePrivate::sendLanguageChanged,
            Qt::QueuedConnection);

    connect(this, &QQmlDebugTranslationServiceImpl::translationIssues,
            d, &QQmlDebugTranslationServicePrivate::sendTranslationIssues,
            Qt::QueuedConnection);

    connect(this, &QQmlDebugTranslationServiceImpl::sendTranslatableTextOccurrences,
            d, &QQmlDebugTranslationServicePrivate::sendTranslatableTextOccurrences,
            Qt::QueuedConnection);
}

QQmlDebugTranslationServiceImpl::~QQmlDebugTranslationServiceImpl()
{
    delete d->proxyTranslator;
    d->proxyTranslator = {};
}

void QQmlDebugTranslationServiceImpl::messageReceived(const QByteArray &message)
{
    QVersionedPacket<QQmlDebugConnector> packet(message);
    QQmlDebugTranslation::Request command;

    packet >> command;
    switch (command) {
        case QQmlDebugTranslation::Request::ChangeLanguage: {
            QUrl context;
            QString locale;
            packet >> context >> locale;
            emit language(context, QLocale(locale));
            break;
        }
        case QQmlDebugTranslation::Request::ChangeState: {
            QString stateName;
            packet >> stateName;
            emit state(stateName);
            break;
        }
        case QQmlDebugTranslation::Request::StateList: {
            emit stateList();
            break;
        }
        case QQmlDebugTranslation::Request::TranslationIssues: {
            emit translationIssues();
            break;
        }
        case QQmlDebugTranslation::Request::TranslatableTextOccurrences: {
            emit sendTranslatableTextOccurrences();
            break;
        }
        case QQmlDebugTranslation::Request::WatchTextElides: {
            emit watchTextElides(true);
            break;
        }
        case QQmlDebugTranslation::Request::DisableWatchTextElides: {
            emit watchTextElides(false);
            break;
        }
        default: {
            qWarning() << "DebugTranslationService: received unknown command: " << static_cast<int>(command);
            break;
        }
    } // switch (command)
}

void QQmlDebugTranslationServiceImpl::engineAboutToBeAdded(QJSEngine *engine)
{
    if (QQmlEngine *qmlEngine = qobject_cast<QQmlEngine *>(engine))
        d->proxyTranslator->addEngine(qmlEngine);

    if (engine->parent())
        d->currentQuickView = qobject_cast<QQuickView*>(engine->parent());

    emit attachedToEngine(engine);
}

void QQmlDebugTranslationServiceImpl::engineAboutToBeRemoved(QJSEngine *engine)
{
    if (QQmlEngine *qmlEngine = qobject_cast<QQmlEngine *>(engine))
        d->proxyTranslator->removeEngine(qmlEngine);
    emit detachedFromEngine(engine);
}

void QQmlDebugTranslationServiceImpl::foundTranslationBinding(const TranslationBindingInformation &translationBindingInformation)
{
    QObject *scopeObject = translationBindingInformation.scopeObject;
    connect(scopeObject, &QObject::destroyed, [this, scopeObject] () {
        this->d->objectTranslationBindingMultiMap.remove(scopeObject);
    });

    d->objectTranslationBindingMultiMap.insert(scopeObject, translationBindingInformation);
}

QT_END_NAMESPACE

#include "moc_qqmldebugtranslationservice.cpp"

#include <qqmldebugtranslationservice.moc>
