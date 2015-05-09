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

void RenderItem::startTest()
{
	int count = point_for_sort.size() - 1;
	sort_type.resize(count);
	double sum = 0.0f;
	for (int i = 0; i < count; ++i)
	{
		glm::dvec2 prev, cur, next;
		std::tie(prev.x, prev.y) = point_for_sort[(i - 1 + count) % count];
		std::tie(cur.x, cur.y) = point_for_sort[i];
		std::tie(next.x, next.y) = point_for_sort[(i + 1) % count];

		glm::dvec2 va = cur - prev;
		glm::dvec2 vb = next - cur;
		float res = va.x * vb.y - vb.x * va.y;
		sum += res;
		float res_is = dist(next, prev) - dist(cur, next) - dist(cur, prev);
		if (fabs(res_is) < 1e-2f)
		{
			sort_type[i] = 0;
		}
		else if (res > 0.0f)
		{
			sort_type[i] = 1;
		}
		else
		{
			sort_type[i] = -1;
		}
	}
	for (int i = 0; i < count; ++i)
	{
		if (sum < 0.0f)
			sort_type[i] *= -1;
		emit drawSortType(point_for_sort[i].first, point_for_sort[i].second, sort_type[i]);
	}
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
