#include "qt/qt_common/qtoglcontextfactory.hpp"

#include "base/assert.hpp"
#include "base/stl_add.hpp"

namespace qt
{
namespace common
{
QtOGLContextFactory::QtOGLContextFactory(QOpenGLContext * rootContext) : m_rootContext(rootContext)
{
  m_uploadSurface = CreateSurface();
  m_drawSurface = CreateSurface();
}

QtOGLContextFactory::~QtOGLContextFactory()
{
  m_drawContext.reset();
  m_uploadContext.reset();

  m_drawSurface->destroy();
  m_uploadSurface->destroy();
}

void QtOGLContextFactory::PrepareToShutdown()
{
  m_preparedToShutdown = true;
}

bool QtOGLContextFactory::LockFrame()
{
  if (m_preparedToShutdown || !m_drawContext)
    return false;

  m_drawContext->lockFrame();
  return true;
}

QRectF const & QtOGLContextFactory::GetTexRect() const
{
  ASSERT(m_drawContext != nullptr, ());
  return m_drawContext->getTexRect();
}

GLuint QtOGLContextFactory::GetTextureHandle() const
{
  ASSERT(m_drawContext != nullptr, ());
  return m_drawContext->getTextureHandle();
}

void QtOGLContextFactory::UnlockFrame()
{
  ASSERT(m_drawContext != nullptr, ());
  m_drawContext->unlockFrame();
}

dp::OGLContext * QtOGLContextFactory::getDrawContext()
{
  if (!m_drawContext)
    m_drawContext = my::make_unique<QtRenderOGLContext>(m_rootContext, m_drawSurface.get());

  return m_drawContext.get();
}

dp::OGLContext * QtOGLContextFactory::getResourcesUploadContext()
{
  if (!m_uploadContext)
    m_uploadContext = my::make_unique<QtUploadOGLContext>(m_rootContext, m_uploadSurface.get());

  return m_uploadContext.get();
}

std::unique_ptr<QOffscreenSurface> QtOGLContextFactory::CreateSurface()
{
  QSurfaceFormat format = m_rootContext->format();
  auto result = my::make_unique<QOffscreenSurface>(m_rootContext->screen());
  result->setFormat(format);
  result->create();
  ASSERT(result->isValid(), ());

  return result;
}
}  // namespace common
}  // namespace qt
