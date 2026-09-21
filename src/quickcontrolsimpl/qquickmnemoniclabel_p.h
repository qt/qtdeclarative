// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

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

class Q_QUICKCONTROLS2IMPL_EXPORT QQuickMnemonicLabel : public QQuickText
{
    Q_OBJECT
    Q_PROPERTY(QString text READ text WRITE setText FINAL)
    Q_PROPERTY(bool mnemonicEnabled READ isMnemonicEnabled WRITE setMnemonicEnabled FINAL)
    QML_NAMED_ELEMENT(MnemonicLabel)
    QML_ADDED_IN_VERSION(2, 3)

public:
    explicit QQuickMnemonicLabel(QQuickItem *parent = nullptr);

    QString text() const;
    void setText(const QString &text) override;

    bool isMnemonicEnabled() const;
    void setMnemonicEnabled(bool enabled);

private:
    void updateMnemonic();

    // True by default because it's the default for most types like Button:
    // only ItemDelegate and its derived types set it to false. Note that underlining
    // is controlled separately; see the docs for this property.
    bool m_mnemonicEnabled = true;
    QString m_fullText;
};

QT_END_NAMESPACE

#endif // QQUICKMNEMONICLABEL_P_H
