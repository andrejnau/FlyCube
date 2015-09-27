#include "renderitem.h"

RenderItem::RenderItem()
	: cur_app_mode(app_mode::draw_fractal)
	, input_polygon(new PrintPolygon())
	, renderer(new Fractal())
{
	connect(this, SIGNAL(windowChanged(QQuickWindow*)), this, SLOT(handleWindowChanged(QQuickWindow*)));
}

void RenderItem::paint_fractal()
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

void RenderItem::cleanup()
{
	input_polygon->destroy();
}

void RenderItem::paint()
{
	if (cur_app_mode == app_mode::draw_fractal)
		paint_fractal();
	else if (cur_app_mode == app_mode::enter_polygon)
		paint_polygon();
}

void RenderItem::sync()
{
	static bool init = false;
	if (!init)
	{
		init = true;
		ogl_LoadFunctions();
		connect(window(), SIGNAL(beforeRendering()), this, SLOT(paint()), Qt::DirectConnection);
	}
}

void RenderItem::doSend_Manual(bool flag)
{
	if (flag == false)
	{
		cur_app_mode = app_mode::draw_fractal;
	}
	else
	{
		input_polygon->clear();
		cur_app_mode = app_mode::enter_polygon;
	}
}

void RenderItem::doSend_Apply(int mode)
{
	if (cur_point.size() >= 2)
	{
		std::vector<line> cur_polygon;
		if (mode == 1)
		{
			auto firts_point = cur_point.front();
			for (auto &p : cur_point)
			{
				p -= firts_point;
			}
			float alpha = atan2(cur_point.back().y, cur_point.back().x);
			for (auto &p : cur_point)
			{
				p = rotate(p, -alpha);
			}

			float r = dist(cur_point.front(), cur_point.back());
			r = 1.0f / r;
			for (auto &p : cur_point)
			{
				p *= r;
			}
		}

		for (int i = 0; i < (int)cur_point.size() - 1; ++i)
		{
			cur_polygon.push_back(line(cur_point[i], cur_point[i + 1]));
		}

		if (mode == 0)
		{
			renderer->set_polygon(std::move(cur_polygon));
		}
		else if (mode == 1)
		{
			renderer->set_template(std::move(cur_polygon));
		}
	}
	else if (mode == 1)
	{
		renderer->set_default_template();
	}
	cur_point.clear();
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
	input_polygon->add_point(cur_point.back());
}

void RenderItem::doSend_Vertex(int val)
{
	teragon setting = renderer->get_setting();
	setting.n = val;
	renderer->set_setting(setting);
}

void RenderItem::doSend_Eps(double val)
{
	teragon setting = renderer->get_setting();
	setting.eps = val;
	renderer->set_setting(setting);
}

void RenderItem::doSend_Iteration(int val)
{
	teragon setting = renderer->get_setting();
	setting.max_it = val;
	renderer->set_setting(setting);
}

void RenderItem::doSend_D(int val)
{
	teragon setting = renderer->get_setting();
	setting.D = val;
	renderer->set_setting(setting);
}

void RenderItem::doSend_Di(int val)
{
	teragon setting = renderer->get_setting();
	setting.Di = val;
	renderer->set_setting(setting);
}

void RenderItem::doSend_D0(int val)
{
	teragon setting = renderer->get_setting();
	setting.D0 = val;
	renderer->set_setting(setting);
}