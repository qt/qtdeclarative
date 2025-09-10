// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#ifndef QQMLDEBUGTRANSLATIONSERVICE_H
#define QQMLDEBUGTRANSLATIONSERVICE_H

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
#include <QtCore/qglobal.h>

#include <private/qqmldebugserviceinterfaces_p.h>

#include <QtCore/qbytearray.h>
#include <QtCore/qstring.h>
#include <QtCore/qurl.h>
#include <QtGui/qcolor.h>

QT_BEGIN_NAMESPACE

class QQmlDebugTranslationServicePrivate;
class QQmlDebugTranslationServiceImpl : public QQmlDebugTranslationService
{
    Q_OBJECT
public:
    QQmlDebugTranslationServiceImpl(QObject *parent = nullptr);
    ~QQmlDebugTranslationServiceImpl();

    void foundTranslationBinding(const TranslationBindingInformation &translationBindingInformation) override;

    void messageReceived(const QByteArray &message) override;
    void engineAboutToBeAdded(QJSEngine *engine) override;
    void engineAboutToBeRemoved(QJSEngine *engine) override;

Q_SIGNALS:
    void language(const QUrl &context, const QLocale &locale);
    void state(const QString &stateName);
    void stateList();
    void watchTextElides(bool);
    void translationIssues();
    void elidedTranslations();
    void sendTranslatableTextOccurrences();

private:
    QQmlDebugTranslationServicePrivate *d;
};

QT_END_NAMESPACE

#endif // QQMLDEBUGTRANSLATIONSERVICE_H
