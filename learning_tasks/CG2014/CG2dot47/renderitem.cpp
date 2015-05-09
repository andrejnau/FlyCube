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
	: m_input(false)
{
	connect(this, SIGNAL(windowChanged(QQuickWindow*)), this, SLOT(handleWindowChanged(QQuickWindow*)));
}

bool RenderItem::isInput() const
{
	return m_input;
}

void RenderItem::setInput(bool val)
{
	if (m_input == val)
		return;

	m_input = val;

	if (m_input)
		cleanScene();

	emit inputChanged(val);
}

std::pair<float, float> RenderItem::pointToGL(int point_x, int point_y)
{
	int m_width = this->window()->width();
	int m_height = this->window()->height();
	float ratio = (float)m_height / (float)m_width;
	float XPixelSize = 2.0f * (1.0f * point_x / m_width);
	float YPixelSize = 2.0f * (1.0f * point_y / m_height);
	float Xposition = XPixelSize - 1.0f;
	float Yposition = (2.0f - YPixelSize) - 1.0f;
	if (m_width >= m_height)
		Xposition /= ratio;
	else
		Yposition *= ratio;

	return{ Xposition, Yposition };
}

void RenderItem::doSendPoint(int point_x, int point_y)
{
	SwapContext set_ctx(window());
	glm::vec2 point;
	std::tie(point.x, point.y) = pointToGL(point_x, point_y);
	if (input_polygon->add_point(point))
	{
		setInput(false);
		if (!point_for_sort.empty())
			std::tie(point_x, point_y) = point_for_sort.front();
	}
	point_for_sort.push_back({ point_x, point_y });

	if (!isInput())
		startTest();
}

void RenderItem::doEndInput()
{
	setInput(false);

	if (!isInput())
		startTest(false);
}

void RenderItem::doSendFuturePoint(int point_x, int point_y)
{
	SwapContext set_ctx(window());
	glm::vec2 point;
	std::tie(point.x, point.y) = pointToGL(point_x, point_y);
	input_polygon->future_point(point);
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

void RenderItem::cleanScene()
{
	SwapContext set_ctx(window());
	input_polygon->clear();
	point_for_sort.clear();
	sort_type.clear();
}

glm::vec2 get_p(const std::vector<glm::vec2> & p, int q, float t)
{
	glm::vec2 res =
		p[q - 1] + ((-p[q - 2] + p[q]) / 2.0f) * t + ((2.0f * p[q - 2] - 5.0f * p[q - 1] + 4.0f * p[q] - p[q + 1]) / 2.0f) * t * t
		+ ((-p[q - 2] + 3.0f * p[q - 1] - 3.0f * p[q] + p[q + 1]) / 2.0f) * t * t * t;
	return res;
}

void RenderItem::startTest(bool f)
{
	int n = point_for_sort.size();
	std::vector<glm::vec2> vec;
	vec.push_back(glm::vec2(point_for_sort[0].first, point_for_sort[0].second));
	for (int i = 0; i < point_for_sort.size(); ++i)
	{
		vec.push_back(glm::vec2(point_for_sort[i].first, point_for_sort[i].second));
	}
	vec.push_back(vec.back());

	std::vector<glm::vec2> out_vec;
	for (int i = 2; i <= n; ++i)
	{
		for (float t = 0.0f; t <= 1.0f; t += 1e-1)
		{
			glm::vec2 point = get_p(vec, i, t);
			std::tie(point.x, point.y) = pointToGL(point.x, point.y);
			out_vec.push_back(point);
		}
	}
	glm::vec4 color = { 0.0f, 0.05f, 1.00f, 1.0f };
	SwapContext set_ctx(window());
	input_polygon->set_Beziers(out_vec, color);
}

void RenderItem::paint()
{
	paintPolygon();
}

void RenderItem::paintPolygon()
{
	SwapContext set_ctx(window());
	input_polygon->draw();
}

void RenderItem::sync()
{
	if (!input_polygon)
	{
		backgroundContext = new QOpenGLContext(this);
		backgroundContext->setFormat(window()->openglContext()->format());
		backgroundContext->create();
		window()->openglContext()->setShareContext(backgroundContext);
	}

	SwapContext ctx(window());
	if (!input_polygon)
	{
		ogl_LoadFunctions();
		input_polygon.reset(new PrintPolygon());
		input_polygon->init();
		connect(window(), SIGNAL(beforeRendering()), this, SLOT(paint()), Qt::DirectConnection);
	}
	input_polygon->resize(0, 0, window()->width(), window()->height());
}
void RenderItem::cleanup()
{
	SwapContext set_ctx(window());
	input_polygon->destroy();
}
