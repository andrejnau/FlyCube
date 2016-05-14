#include "renderitem.h"

class SwapContext
{
public:
	SwapContext(QQuickWindow *_win)
		: win(_win)
	{
		win->openglContext()->shareContext()->makeCurrent(win);
	}
	~SwapContext()
	{
		win->openglContext()->makeCurrent(win);
	}
private:
	QQuickWindow *win;
};

RenderItem::RenderItem()
{
	connect(this, SIGNAL(windowChanged(QQuickWindow*)), this, SLOT(handleWindowChanged(QQuickWindow*)));
}

void RenderItem::handleWindowChanged(QQuickWindow *win)
{
	if (win)
	{
		connect(win, SIGNAL(beforeSynchronizing()), this, SLOT(sync()), Qt::DirectConnection);
		connect(win, SIGNAL(sceneGraphInvalidated()), this, SLOT(cleanup()), Qt::DirectConnection);
		win->setClearBeforeRendering(false);
	}
}

void RenderItem::paint()
{
	SwapContext ctx(window());
	renderer->draw();
}

void RenderItem::sync()
{
	if (!renderer)
	{
		backgroundContext = new QOpenGLContext(this);
		backgroundContext->setFormat(window()->openglContext()->format());
		backgroundContext->create();
		window()->openglContext()->setShareContext(backgroundContext);
	}

	SwapContext ctx(window());
	if (!renderer)
	{
		glbinding::Binding::initialize();
		renderer.reset(new TestScene());
		renderer->init();
		connect(window(), SIGNAL(beforeRendering()), this, SLOT(paint()), Qt::DirectConnection);
	}
	renderer->resize(0, 0, window()->width(), window()->height());
}

void RenderItem::cleanup()
{
	if (renderer)
	{
		renderer->destroy();
		renderer.reset(nullptr);
	}
}