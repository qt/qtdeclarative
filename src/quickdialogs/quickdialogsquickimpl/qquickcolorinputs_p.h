// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQUICKCOLORINPUTS_P_H
#define QQUICKCOLORINPUTS_P_H

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

#include <QtGui/qcolor.h>
#include <QtQuick/private/qquickitem_p.h>
#include <QtQuickTemplates2/private/qquicktextfield_p.h>

#include "qtquickdialogs2quickimplglobal_p.h"

#include "qquickcolordialogutils_p.h"

#include <QtCore/qpointer.h>

QT_BEGIN_NAMESPACE

class Q_QUICKDIALOGS2QUICKIMPL_PRIVATE_EXPORT QQuickColorInputs : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged)
    Q_PROPERTY(int red READ red NOTIFY colorChanged)
    Q_PROPERTY(int green READ green NOTIFY colorChanged)
    Q_PROPERTY(int blue READ blue NOTIFY colorChanged)
    Q_PROPERTY(qreal hue READ hue NOTIFY colorChanged)
    Q_PROPERTY(qreal hslSaturation READ hslSaturation NOTIFY colorChanged)
    Q_PROPERTY(qreal hsvSaturation READ hsvSaturation NOTIFY colorChanged)
    Q_PROPERTY(qreal value READ value NOTIFY colorChanged)
    Q_PROPERTY(qreal lightness READ lightness NOTIFY colorChanged)
    Q_PROPERTY(qreal alpha READ alpha NOTIFY colorChanged)
    Q_PROPERTY(bool showAlpha READ showAlpha WRITE setShowAlpha NOTIFY showAlphaChanged)
    Q_PROPERTY(QQuickTextInput *hexInput READ hexInput WRITE setHexInput NOTIFY hexInputChanged)
    Q_PROPERTY(QQuickTextInput *redInput READ redInput WRITE setRedInput NOTIFY redInputChanged)
    Q_PROPERTY(QQuickTextInput *greenInput READ greenInput WRITE setGreenInput NOTIFY greenInputChanged)
    Q_PROPERTY(QQuickTextInput *blueInput READ blueInput WRITE setBlueInput NOTIFY blueInputChanged)
    Q_PROPERTY(QQuickTextInput *hsvHueInput READ hsvHueInput WRITE setHsvHueInput NOTIFY hsvHueInputChanged)
    Q_PROPERTY(QQuickTextInput *hslHueInput READ hslHueInput WRITE setHslHueInput NOTIFY hslHueInputChanged)
    Q_PROPERTY(QQuickTextInput *hsvSaturationInput READ hsvSaturationInput WRITE setHsvSaturationInput NOTIFY hsvSaturationInputChanged)
    Q_PROPERTY(QQuickTextInput *hslSaturationInput READ hslSaturationInput WRITE setHslSaturationInput NOTIFY hslSaturationInputChanged)
    Q_PROPERTY(QQuickTextInput *valueInput READ valueInput WRITE setValueInput NOTIFY valueInputChanged)
    Q_PROPERTY(QQuickTextInput *lightnessInput READ lightnessInput WRITE setLightnessInput NOTIFY lightnessInputChanged)
    Q_PROPERTY(QQuickTextInput *rgbAlphaInput READ rgbAlphaInput WRITE setRgbAlphaInput NOTIFY rgbAlphaInputChanged)
    Q_PROPERTY(QQuickTextInput *hsvAlphaInput READ hsvAlphaInput WRITE setHsvAlphaInput NOTIFY hsvAlphaInputChanged)
    Q_PROPERTY(QQuickTextInput *hslAlphaInput READ hslAlphaInput WRITE setHslAlphaInput NOTIFY hslAlphaInputChanged)
    QML_NAMED_ELEMENT(ColorInputsImpl)

public:
    explicit QQuickColorInputs();

    QColor color() const;
    void setColor(const QColor &c);
    int red() const;
    int green() const;
    int blue() const;
    qreal alpha() const;
    qreal hue() const;
    qreal hslSaturation() const;
    qreal hsvSaturation() const;
    qreal value() const;
    qreal lightness() const;

    bool showAlpha() const;
    void setShowAlpha(bool showAlpha);

    QQuickTextInput *hexInput() const;
    void setHexInput(QQuickTextInput *hexInput);

    QQuickTextInput *redInput() const;
    void setRedInput(QQuickTextInput *redInput);

    QQuickTextInput *greenInput() const;
    void setGreenInput(QQuickTextInput *greenInput);

    QQuickTextInput *blueInput() const;
    void setBlueInput(QQuickTextInput *blueInput);

    QQuickTextInput *hsvHueInput() const;
    void setHsvHueInput(QQuickTextInput *hsvHueInput);

    QQuickTextInput *hslHueInput() const;
    void setHslHueInput(QQuickTextInput *hslHueInput);

    QQuickTextInput *hsvSaturationInput() const;
    void setHsvSaturationInput(QQuickTextInput *hsvSaturationInput);

    QQuickTextInput *hslSaturationInput() const;
    void setHslSaturationInput(QQuickTextInput *hslSaturationInput);

    QQuickTextInput *valueInput() const;
    void setValueInput(QQuickTextInput *valueInput);

    QQuickTextInput *lightnessInput() const;
    void setLightnessInput(QQuickTextInput *lightnessInput);

    QQuickTextInput *rgbAlphaInput() const;
    void setRgbAlphaInput(QQuickTextInput *alphaInput);

    QQuickTextInput *hsvAlphaInput() const;
    void setHsvAlphaInput(QQuickTextInput *alphaInput);

    QQuickTextInput *hslAlphaInput() const;
    void setHslAlphaInput(QQuickTextInput *alphaInput);

Q_SIGNALS:
    void colorChanged(const QColor &c);
    void colorModified(const QColor &c);
    void hslChanged();
    void showAlphaChanged(bool);
    void hexInputChanged();
    void redInputChanged();
    void greenInputChanged();
    void blueInputChanged();
    void hsvHueInputChanged();
    void hslHueInputChanged();
    void hsvSaturationInputChanged();
    void hslSaturationInputChanged();
    void valueInputChanged();
    void lightnessInputChanged();
    void rgbAlphaInputChanged();
    void hsvAlphaInputChanged();
    void hslAlphaInputChanged();

private:
    void handleHexChanged();
    void handleRedChanged();
    void handleGreenChanged();
    void handleBlueChanged();
    void handleHsvHueChanged();
    void handleHslHueChanged();
    void handleHueChanged(const QString &input);
    void handleHsvSaturationChanged();
    void handleHslSaturationChanged();
    void handleSaturationChanged(const QString &input);
    void handleValueChanged();
    void handleLightnessChanged();
    void handleRgbAlphaChanged();
    void handleHsvAlphaChanged();
    void handleHslAlphaChanged();
    void handleAlphaChanged(const QString &input);

    QPointer<QQuickTextInput> m_hexInput;
    QPointer<QQuickTextInput> m_redInput;
    QPointer<QQuickTextInput> m_greenInput;
    QPointer<QQuickTextInput> m_blueInput;
    QPointer<QQuickTextInput> m_hsvHueInput;
    QPointer<QQuickTextInput> m_hslHueInput;
    QPointer<QQuickTextInput> m_hsvSaturationInput;
    QPointer<QQuickTextInput> m_hslSaturationInput;
    QPointer<QQuickTextInput> m_valueInput;
    QPointer<QQuickTextInput> m_lightnessInput;
    QPointer<QQuickTextInput> m_rgbAlphaInput;
    QPointer<QQuickTextInput> m_hsvAlphaInput;
    QPointer<QQuickTextInput> m_hslAlphaInput;
    HSVA m_hsva;
    bool m_showAlpha = false;
};

QT_END_NAMESPACE

#endif // QQUICKCOLORINPUTS_P_H
