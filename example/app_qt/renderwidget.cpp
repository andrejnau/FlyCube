#include "renderwidget.h"

RenderWidget::RenderWidget(QWidget *parent) :
QOpenGLWidget(parent), renderer(GetScene::get(SceneType::TestScene))
{
}

RenderWidget::~RenderWidget()
{
	cleanup();
}

void RenderWidget::initializeGL()
{
	ogl_LoadFunctions();
	renderer.init();
	connect(this, SIGNAL(sceneGraphInvalidated()), this, SLOT(cleanup()), Qt::DirectConnection);
}

void RenderWidget::paintGL()
{
	renderer.draw();
	update();
}

void RenderWidget::resizeGL(int nWidth, int nHeight)
{
	renderer.resize(0, 0, nWidth, nHeight);
}

void RenderWidget::cleanup()
{
	renderer.destroy();
}