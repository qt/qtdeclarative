#include "pixmaptexture.h"

PixmapTexture::PixmapTexture(const QImage &image)
    : m_pixmap(QPixmap::fromImage(image))
{
}


int PixmapTexture::textureId() const
{
    return 0;
}

QSize PixmapTexture::textureSize() const
{
    return m_pixmap.size();
}

bool PixmapTexture::hasAlphaChannel() const
{
    return m_pixmap.hasAlphaChannel();
}

bool PixmapTexture::hasMipmaps() const
{
    return false;
}

void PixmapTexture::bind()
{
    Q_UNREACHABLE();
}
