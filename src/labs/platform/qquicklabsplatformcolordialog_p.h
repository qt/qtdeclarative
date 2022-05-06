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

#ifndef QQUICKLABSPLATFORMCOLORDIALOG_P_H
#define QQUICKLABSPLATFORMCOLORDIALOG_P_H

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
#include <QtGui/qcolor.h>
#include <QtQml/qqml.h>

QT_BEGIN_NAMESPACE

class QQuickLabsPlatformColorDialog : public QQuickLabsPlatformDialog
{
    Q_OBJECT
    Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged FINAL)
    Q_PROPERTY(QColor currentColor READ currentColor WRITE setCurrentColor NOTIFY currentColorChanged FINAL)
    Q_PROPERTY(QColorDialogOptions::ColorDialogOptions options READ options WRITE setOptions NOTIFY optionsChanged FINAL)
    Q_FLAGS(QColorDialogOptions::ColorDialogOptions)

public:
    explicit QQuickLabsPlatformColorDialog(QObject *parent = nullptr);

    QColor color() const;
    void setColor(const QColor &color);

    QColor currentColor() const;
    void setCurrentColor(const QColor &color);

    QColorDialogOptions::ColorDialogOptions options() const;
    void setOptions(QColorDialogOptions::ColorDialogOptions options);

Q_SIGNALS:
    void colorChanged();
    void currentColorChanged();
    void optionsChanged();

protected:
    bool useNativeDialog() const override;
    void onCreate(QPlatformDialogHelper *dialog) override;
    void onShow(QPlatformDialogHelper *dialog) override;
    void accept() override;

private:
    QColor m_color;
    QColor m_currentColor; // TODO: QColorDialogOptions::initialColor
    QSharedPointer<QColorDialogOptions> m_options;
};

QT_END_NAMESPACE

QML_DECLARE_TYPE(QQuickLabsPlatformColorDialog)

#endif // QQUICKLABSPLATFORMCOLORDIALOG_P_H
