/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

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
    static QString translationFromInformation(const TranslationBindingInformation &translationBindingInformation);
    static QString originStringFromInformation(const TranslationBindingInformation &translationBindingInformation);
    static QQmlSourceLocation sourceLocationFromInformation(const TranslationBindingInformation &translationBindingInformation);
signals:
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
