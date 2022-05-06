/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Quick Dialogs module of the Qt Toolkit.
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
******************************************************************************/

#ifndef QQUICKFONTDIALOG_P_H
#define QQUICKFONTDIALOG_P_H

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

#include "qquickabstractdialog_p.h"

#include <QtGui/qfont.h>

QT_BEGIN_NAMESPACE

class Q_QUICKDIALOGS2_PRIVATE_EXPORT QQuickFontDialog : public QQuickAbstractDialog
{
    Q_OBJECT
    Q_PROPERTY(QFont selectedFont READ selectedFont WRITE setSelectedFont NOTIFY selectedFontChanged)
    Q_PROPERTY(QFont currentFont READ currentFont WRITE setCurrentFont NOTIFY currentFontChanged FINAL)
    Q_PROPERTY(QFontDialogOptions::FontDialogOptions options READ options WRITE setOptions
               RESET resetOptions NOTIFY optionsChanged)
    Q_FLAGS(QFontDialogOptions::FontDialogOptions)
    QML_NAMED_ELEMENT(FontDialog)
    QML_ADDED_IN_VERSION(6, 2)

public:
    explicit QQuickFontDialog(QObject *parent = nullptr);

    void setCurrentFont(const QFont &font);
    QFont currentFont() const;

    void setSelectedFont(const QFont &font);
    QFont selectedFont() const;

    QFontDialogOptions::FontDialogOptions options() const;
    void setOptions(QFontDialogOptions::FontDialogOptions options);
    void resetOptions();

Q_SIGNALS:
    void selectedFontChanged();
    void currentFontChanged();
    void optionsChanged();

protected:
    bool useNativeDialog() const override;
    void onCreate(QPlatformDialogHelper *dialog) override;
    void onShow(QPlatformDialogHelper *dialog) override;
    void accept() override;

private:
    QFont m_selectedFont;
    QSharedPointer<QFontDialogOptions> m_options;
};

QT_END_NAMESPACE

QML_DECLARE_TYPE(QQuickFontDialog)

#endif // QQUICKFONTDIALOG_P_H
