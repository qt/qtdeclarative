/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Labs Platform module of the Qt Toolkit.
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

#ifndef QQUICKLABSPLATFORMFONTDIALOG_P_H
#define QQUICKLABSPLATFORMFONTDIALOG_P_H

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

#include "qquicklabsplatformdialog_p.h"
#include <QtGui/qfont.h>
#include <QtQml/qqml.h>

QT_BEGIN_NAMESPACE

class QQuickLabsPlatformFontDialog : public QQuickLabsPlatformDialog
{
    Q_OBJECT
    Q_PROPERTY(QFont font READ font WRITE setFont NOTIFY fontChanged FINAL)
    Q_PROPERTY(QFont currentFont READ currentFont WRITE setCurrentFont NOTIFY currentFontChanged FINAL)
    Q_PROPERTY(QFontDialogOptions::FontDialogOptions options READ options WRITE setOptions NOTIFY optionsChanged FINAL)
    Q_FLAGS(QFontDialogOptions::FontDialogOptions)

public:
    explicit QQuickLabsPlatformFontDialog(QObject *parent = nullptr);

    QFont font() const;
    void setFont(const QFont &font);

    QFont currentFont() const;
    void setCurrentFont(const QFont &font);

    QFontDialogOptions::FontDialogOptions options() const;
    void setOptions(QFontDialogOptions::FontDialogOptions options);

Q_SIGNALS:
    void fontChanged();
    void currentFontChanged();
    void optionsChanged();

protected:
    bool useNativeDialog() const override;
    void onCreate(QPlatformDialogHelper *dialog) override;
    void onShow(QPlatformDialogHelper *dialog) override;
    void accept() override;

private:
    QFont m_font;
    QFont m_currentFont; // TODO: QFontDialogOptions::initialFont
    QSharedPointer<QFontDialogOptions> m_options;
};

QT_END_NAMESPACE

QML_DECLARE_TYPE(QQuickLabsPlatformFontDialog)

#endif // QQUICKLABSPLATFORMFONTDIALOG_P_H
