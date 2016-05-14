#include "renderwidget.h"

RenderWidget::RenderWidget(QWidget *parent)
	: QOpenGLWidget(parent)
{
}

RenderWidget::~RenderWidget()
{
	cleanup();
}

void RenderWidget::initializeGL()
{
	glbinding::Binding::initialize();
	if (!renderer)
	{
		renderer.reset(new TestScene());
	}
	renderer->init();
	connect(this, SIGNAL(sceneGraphInvalidated()), this, SLOT(cleanup()), Qt::DirectConnection);
}

void RenderWidget::paintGL()
{
	renderer->draw();
	update();
}

void RenderWidget::resizeGL(int nWidth, int nHeight)
{
	renderer->resize(0, 0, nWidth, nHeight);
}

void RenderWidget::cleanup()
{
	renderer->destroy();
}