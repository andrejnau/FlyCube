#include "renderitem.h"

class SwapContext
{
public:
	SwapContext(QOpenGLContext *ctx, QSurface *surf)
	{
		p_ctx = QOpenGLContext::currentContext();
		p_surf = p_ctx->surface();
		ctx->makeCurrent(surf);
	}
	~SwapContext()
	{
		p_ctx->makeCurrent(p_surf);
	}

private:
	QOpenGLContext *p_ctx;
	QSurface *p_surf;
};

RenderItem::RenderItem() : renderer(TestScene::Instance())
{
	connect(this, SIGNAL(windowChanged(QQuickWindow*)), this, SLOT(handleWindowChanged(QQuickWindow*)));
	ctx.reset(new QOpenGLContext(this));
	ctx->create();
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
	SwapContext set_ctx(ctx.get(), window());
	renderer.draw();
}

void RenderItem::sync()
{
	static bool initSt = false;
	if (!initSt)
	{
		ogl_LoadFunctions();
		connect(window(), SIGNAL(beforeRendering()), this, SLOT(paint()), Qt::DirectConnection);
		SwapContext set_ctx(ctx.get(), window());
		initSt = renderer.init();
	}

	SwapContext set_ctx(ctx.get(), window());
	renderer.resize(0, 0, window()->width(), window()->height());
}

void RenderItem::cleanup()
{
	renderer.destroy();
}