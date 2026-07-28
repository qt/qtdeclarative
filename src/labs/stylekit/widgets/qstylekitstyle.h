// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QSTYLEKITSTYLE_H
#define QSTYLEKITSTYLE_H

#include <QtWidgets/qcommonstyle.h>
#include <QtLabsStyleKit/qtlabsstylekitexports.h>

QT_BEGIN_NAMESPACE

class QStyleKitStylePrivate;
class Q_LABSSTYLEKIT_EXPORT QStyleKitStyle : public QCommonStyle
{
    Q_OBJECT
    Q_PROPERTY(QString stylePath READ stylePath WRITE setStylePath NOTIFY stylePathChanged FINAL)
    Q_PROPERTY(QString themeName READ themeName WRITE setThemeName NOTIFY themeNameChanged FINAL)
    Q_PROPERTY(QStringList availableThemeNames READ availableThemeNames NOTIFY stylePathChanged FINAL)
    Q_PROPERTY(QStringList customThemeNames READ customThemeNames NOTIFY stylePathChanged FINAL)

public:
    QStyleKitStyle();
    explicit QStyleKitStyle(const QString &filePath);
    ~QStyleKitStyle() override;

    QString stylePath() const;
    void setStylePath(const QString &filePath);
    QString themeName() const;
    void setThemeName(const QString &themeName);
    QStringList availableThemeNames() const;
    QStringList customThemeNames() const;

    void drawPrimitive(PrimitiveElement pe, const QStyleOption *opt, QPainter *p,
                       const QWidget *w = nullptr) const override;
    void drawControl(ControlElement element, const QStyleOption *opt, QPainter *p,
                     const QWidget *w = nullptr) const override;
    QRect subElementRect(SubElement r, const QStyleOption *opt, const QWidget *widget = nullptr) const override;
    void drawComplexControl(ComplexControl cc, const QStyleOptionComplex *opt, QPainter *p,
                            const QWidget *w = nullptr) const override;
    SubControl hitTestComplexControl(ComplexControl cc, const QStyleOptionComplex *opt,
                                     const QPoint &pt, const QWidget *w = nullptr) const override;
    QRect subControlRect(ComplexControl cc, const QStyleOptionComplex *opt, SubControl sc,
                         const QWidget *w = nullptr) const override;
    QSize sizeFromContents(ContentsType ct, const QStyleOption *opt,
                           const QSize &contentsSize, const QWidget *widget = nullptr) const override;

    int pixelMetric(PixelMetric m, const QStyleOption *opt = nullptr, const QWidget *widget = nullptr) const override;

    int styleHint(StyleHint sh, const QStyleOption *opt = nullptr, const QWidget *w = nullptr,
                  QStyleHintReturn *shret = nullptr) const override;

    QPalette standardPalette() const override;
    void polish(QWidget *widget) override;
    void polish(QApplication *application) override;
    void polish(QPalette &palette) override;
    void unpolish(QWidget *widget) override;
    void unpolish(QApplication *application) override;

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    bool event(QEvent *event) override;

Q_SIGNALS:
    void stylePathChanged();
    void themeNameChanged();

private:
    Q_DISABLE_COPY_MOVE(QStyleKitStyle)
    Q_DECLARE_PRIVATE(QStyleKitStyle)
};

QT_END_NAMESPACE

#endif // QSTYLEKITSTYLE_H
