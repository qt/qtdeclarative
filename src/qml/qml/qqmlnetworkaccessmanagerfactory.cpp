// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqmlnetworkaccessmanagerfactory.h"

QT_BEGIN_NAMESPACE

#if QT_CONFIG(qml_network)

/*!
    \class QQmlNetworkAccessManagerFactory
    \since 5.0
    \inmodule QtQml
    \brief The QQmlNetworkAccessManagerFactory class creates QNetworkAccessManager instances for a QML engine.

    A QML engine uses QNetworkAccessManager for all network access.
    By implementing a factory, it is possible to provide the QML engine
    with custom QNetworkAccessManager instances with specialized caching,
    proxy and cookies support.

    \list
        \li The QNetworkDiskCache can be used as a request cache with \l {QNetworkDiskCache}.
        \li Using \l {QNetworkProxy}, traffic sent by the QNetworkAccessManager can be tunnelled through a proxy.
        \li Cookies can be saved for future requests by adding a \l {QNetworkCookieJar}.
    \endlist

    To implement a factory, subclass QQmlNetworkAccessManagerFactory and
    implement the virtual create() method, then assign it to the relevant QML
    engine using QQmlEngine::setNetworkAccessManagerFactory(). For instance, the QNetworkAccessManager
    objects created by the following snippet will cache requests.
    \snippet code/src_network_access_qnetworkaccessmanager.cpp 0

    The factory can then be passed to the QML engine so it can instantiate the QNetworkAccessManager with the custom behavior.
    \snippet code/src_network_access_qnetworkaccessmanager.cpp 1

    Note the QML engine may create QNetworkAccessManager instances
    from multiple threads. Because of this, the implementation of the create()
    method must be \l{Reentrancy and Thread-Safety}{reentrant}. In addition,
    the developer should be careful if the signals of the object to be
    returned from create() are connected to the slots of an object that may
    be created in a different thread:

    \list
    \li The QML engine internally handles all requests, and cleans up any
       QNetworkReply objects it creates. Receiving the
       QNetworkAccessManager::finished() signal in another thread may not
       provide the receiver with a valid reply object if it has already
       been deleted.
    \li Authentication details provided to QNetworkAccessManager::authenticationRequired()
       must be provided immediately, so this signal cannot be connected as a
       Qt::QueuedConnection (or as the default Qt::AutoConnection from another
       thread).
    \endlist

    For more information about signals and threads, see
    \l {Threads and QObjects} and \l {Signals and Slots Across Threads}.

    \sa QNetworkDiskCache
*/

/*!
    Destroys the factory. The default implementation does nothing.
 */
QQmlNetworkAccessManagerFactory::~QQmlNetworkAccessManagerFactory()
{
}

/*!
    \fn QNetworkAccessManager *QQmlNetworkAccessManagerFactory::create(QObject *parent)

    Creates and returns a network access manager with the specified \a parent.
    This method must return a new QNetworkAccessManager instance each time
    it is called.

    Note: this method may be called by multiple threads, so ensure the
    implementation of this method is reentrant.
*/

#endif // qml_network

QT_END_NAMESPACE
