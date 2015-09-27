#include "renderitem.h"

RenderItem::RenderItem()
	: cur_app_mode(app_mode::enter_polygon)
{
	connect(this, SIGNAL(windowChanged(QQuickWindow*)), this, SLOT(handleWindowChanged(QQuickWindow*)));
}

void RenderItem::paint_image()
{
	static bool initSt = false;
	if (!initSt)
	{
		renderer->init();
		initSt = true;
	}
	renderer->resize(this->x(), this->y(), this->width(), this->height());
	renderer->draw();
}

void RenderItem::paint_polygon()
{
	static bool initSt = false;
	if (!initSt)
	{
		input_polygon->init();
		initSt = true;
	}
	input_polygon->resize(this->x(), this->y(), this->width(), this->height());
	input_polygon->draw();
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
	if (cur_app_mode == app_mode::draw_image)
		paint_image();
	else if (cur_app_mode == app_mode::enter_polygon)
		paint_polygon();
}

void RenderItem::cleanup()
{
	input_polygon->destroy();
}

void RenderItem::sync()
{
	if (!input_polygon)
	{
		ogl_LoadFunctions();
		input_polygon.reset(new PrintPolygon());
	}

	if (!renderer)
	{
		renderer.reset(new PrintImage());
		connect(window(), SIGNAL(beforeRendering()), this, SLOT(paint()), Qt::DirectConnection);
	}
}

void RenderItem::doSend_nextPoint(int point_x, int point_y)
{
	int m_width = this->width() - this->x();
	int m_height = this->height() - this->y();
	GLfloat ratio = (GLfloat)m_height / (GLfloat)m_width;
	float XPixelSize = 2.0f * (1.0f * point_x / m_width);
	float YPixelSize = 2.0f * (1.0f * point_y / m_height);
	float Xposition = XPixelSize - 1.0f;
	float Yposition = (2.0f - YPixelSize) - 1.0f;
	if (m_width >= m_height)
		Xposition /= ratio;
	else
		Yposition *= ratio;

	input_polygon->next_point(glm::vec2(Xposition, Yposition));
}

void RenderItem::doSend_Manual(bool flag)
{
	if (flag)
	{
		input_polygon->clear();
		cur_point.clear();
		input_polygon->beginInpit();
		cur_app_mode = app_mode::enter_polygon;
	}
}

void RenderItem::doSend_Apply()
{
	glm::vec2 a = cur_point[0] - cur_point[1];
	glm::vec2 b = cur_point[2] - cur_point[1];
	cur_point.push_back(cur_point[1] + (a + b));
	input_polygon->next_point(cur_point.back());

	cur_point.push_back(cur_point.front());
	input_polygon->next_point(cur_point.back());

	qDebug() << "void RenderItem::doSend_Apply()";
	QImage map("://image.jpg");
	renderer->setImage(map, this->cur_point);
	cur_app_mode = app_mode::draw_image;

	input_polygon->clear();
	cur_point.clear();
	input_polygon->beginInpit();

}

void RenderItem::doSend_Point(int point_x, int point_y)
{
	int m_width = this->width() - this->x();
	int m_height = this->height() - this->y();
	GLfloat ratio = (GLfloat)m_height / (GLfloat)m_width;
	float XPixelSize = 2.0f * (1.0f * point_x / m_width);
	float YPixelSize = 2.0f * (1.0f * point_y / m_height);
	float Xposition = XPixelSize - 1.0f;
	float Yposition = (2.0f - YPixelSize) - 1.0f;
	if (m_width >= m_height)
		Xposition /= ratio;
	else
		Yposition *= ratio;
	cur_point.push_back(glm::vec2(Xposition, Yposition));
	float eps = 30.0 / std::max(m_width, m_height);
	if (cur_point.size() > 1 && glm::distance(cur_point.front(), cur_point.back()) < eps)
	{
		cur_point.pop_back();
		cur_point.push_back(cur_point.front());
		input_polygon->add_point(cur_point.back());
		input_polygon->endInpit();

	}
	else
		input_polygon->add_point(cur_point.back());
}

int RenderItem::getSize()
{
	return cur_point.size();
}

bool RenderItem::isCorrectPolugon()
{
	std::vector<float> sing;
	for (int i = 0; i < (int)cur_point.size(); ++i)
	{
		glm::vec2 next1 = cur_point[(i + 1) % cur_point.size()];
		glm::vec2 next2 = cur_point[(i + 2) % cur_point.size()];

		glm::vec2 vec1 = next1 - cur_point[i];
		glm::vec2 vec2 = next2 - cur_point[i];

		sing.push_back(vec1.x * vec2.y - vec1.y * vec2.x);
	}
	bool f1 = true, f2 = true;
	for (int i = 0; i < (int)sing.size(); ++i)
	{
		if (!sing[i])
			continue;
		f1 = f1 & (sing[i] < 0);
		f2 = f2 & (sing[i] > 0);
	}

	return (f1 ^ f2);

}

bool RenderItem::isClosed()
{
	return cur_point.size() > 1 && cur_point.front() == cur_point.back();
}