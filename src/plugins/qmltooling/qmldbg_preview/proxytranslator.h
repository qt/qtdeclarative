// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef PROXYTRANSLATOR_H
#define PROXYTRANSLATOR_H

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

#include <private/qqmldebugserviceinterfaces_p.h>
#include <private/qqmlglobal_p.h>

#include <QtCore/qstring.h>
#include <QtCore/qurl.h>
#include <QtCore/qtranslator.h>

#include <memory>

QT_BEGIN_NAMESPACE

class ProxyTranslator : public QTranslator
{
    Q_OBJECT
public:
    QString translate(const char *context, const char *sourceText, const char *disambiguation, int n) const override;
    bool isEmpty() const override;

    QString currentUILanguages() const;
    void setLanguage(const QUrl &context, const QLocale &locale);
    void addEngine(QQmlEngine *engine);
    void removeEngine(QQmlEngine *engine);

    bool hasTranslation(const TranslationBindingInformation &translationBindingInformation) const;
    static QString
    translationFromInformation(const TranslationBindingInformation &translationBindingInformation);
    static QQmlSourceLocation sourceLocationFromInformation(const TranslationBindingInformation &translationBindingInformation);
Q_SIGNALS:
    void languageChanged(const QLocale &locale);

private:
    void resetTranslationFound() const;
    bool translationFound() const;
    QList<QQmlEngine *> m_engines;
    std::unique_ptr<QTranslator> m_qtTranslator;
    std::unique_ptr<QTranslator> m_qmlTranslator;
    bool m_enable = false;
    QString m_currentUILanguages;
    mutable bool m_translationFound = false;
};

QT_END_NAMESPACE

#endif // PROXYTRANSLATOR_H
