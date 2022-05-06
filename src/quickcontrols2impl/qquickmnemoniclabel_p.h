/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Quick Controls 2 module of the Qt Toolkit.
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

#ifndef QQUICKMNEMONICLABEL_P_H
#define QQUICKMNEMONICLABEL_P_H

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

#include <QtQuick/private/qquicktext_p.h>
#include <QtQuickControls2Impl/private/qtquickcontrols2implglobal_p.h>

QT_BEGIN_NAMESPACE

class Q_QUICKCONTROLS2_PRIVATE_EXPORT QQuickMnemonicLabel : public QQuickText
{
    Q_OBJECT
    Q_PROPERTY(QString text READ text WRITE setText FINAL)
    Q_PROPERTY(bool mnemonicVisible READ isMnemonicVisible WRITE setMnemonicVisible FINAL)
    QML_NAMED_ELEMENT(MnemonicLabel)
    QML_ADDED_IN_VERSION(2, 3)

public:
    explicit QQuickMnemonicLabel(QQuickItem *parent = nullptr);

    QString text() const;
    void setText(const QString &text);

    bool isMnemonicVisible() const;
    void setMnemonicVisible(bool visible);

private:
    void updateMnemonic();

    bool m_mnemonicVisible = true;
    QString m_fullText;
};

QT_END_NAMESPACE

QML_DECLARE_TYPE(QQuickMnemonicLabel)

#endif // QQUICKMNEMONICLABEL_P_H
