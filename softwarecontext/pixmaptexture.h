#ifndef PIXMAPTEXTURE_H
#define PIXMAPTEXTURE_H

#include <private/qsgtexture_p.h>

class PixmapTexture : public QSGTexture
{
    Q_OBJECT
public:
    PixmapTexture(const QImage &image);

    virtual int textureId() const;
    virtual QSize textureSize() const;
    virtual bool hasAlphaChannel() const;
    virtual bool hasMipmaps() const;
    virtual void bind();

    QPixmap pixmap() const { return m_pixmap; }

private:
    QPixmap m_pixmap;
};

#endif // PIXMAPTEXTURE_H
