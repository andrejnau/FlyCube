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

RenderItem::RenderItem() : input_polygon(PrintPolygon::Instance()), m_input(false)
{
	connect(this, SIGNAL(windowChanged(QQuickWindow*)), this, SLOT(handleWindowChanged(QQuickWindow*)));
	ctx.reset(new QOpenGLContext(this));
	ctx->create();
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
	SwapContext set_ctx(ctx.get(), window());
	glm::vec2 point;
	std::tie(point.x, point.y) = pointToGL(point_x, point_y);
	if (input_polygon.add_point(point))
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
	SwapContext set_ctx(ctx.get(), window());
	glm::vec2 point;
	std::tie(point.x, point.y) = pointToGL(point_x, point_y);
	input_polygon.future_point(point);
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
	SwapContext set_ctx(ctx.get(), window());
	input_polygon.clear();
	point_for_sort.clear();
	sort_type.clear();
}

void RenderItem::startTest(bool f)
{
    int n = point_for_sort.size() - f;
    std::vector<std::vector<double> > u(2, std::vector<double>(n));
	std::vector<std::vector<double> > u_res(2, std::vector<double>(n));
	for (int i = 0; i < n; ++i)
	{
		std::tie(u[0][i], u[1][i]) = point_for_sort[i];
	}
	u_res = u *	getInverseMatrix(getHi(n)) * getD(n) * getInverseMatrix(getHb(n));

	std::vector<glm::vec2> out_vec;
    for (int i = 0; i < n; ++i)
    {
        glm::vec2 point;
        std::tie(point.x, point.y) = pointToGL(u_res[0][i], u_res[1][i]);
		out_vec.push_back(point);
    }
	glm::vec4 color(1.0f, 0.0f, 0.05f, 1.0f);
	SwapContext set_ctx(ctx.get(), window());
	input_polygon.set_based(out_vec, color);

	std::vector<glm::vec2> out_Beziers;
	double eps = 1e-4;
	for (double t = 0.0; t <= 1.0; t += eps)
	{
		glm::vec2 point = {0.0f, 0.0f};
		for (int i = 0; i < n; ++i)
		{
			glm::vec2 u_point(u_res[0][i], u_res[1][i]);
			float b = getC(i, n-1) * binpow(t, i) * binpow((1.0 - t), n-1 - i);
			point += u_point * b;
		}
		std::tie(point.x, point.y) = pointToGL(point.x, point.y);
		out_Beziers.push_back(point);
	}

	color = { 0.0f, 0.05f, 1.00f, 1.0f };
	input_polygon.set_Beziers(out_Beziers, color);
}

void RenderItem::paint()
{
	paintPolygon();
}

void RenderItem::paintPolygon()
{
	SwapContext set_ctx(ctx.get(), window());
	input_polygon.draw();
}

void RenderItem::sync()
{
	SwapContext set_ctx(ctx.get(), window());
	static bool initSt = false;
	if (!initSt)
	{
		ogl_LoadFunctions();
		connect(window(), SIGNAL(beforeRendering()), this, SLOT(paint()), Qt::DirectConnection);
		initSt = input_polygon.init();
	}
	input_polygon.resize(0, 0, window()->width(), window()->height());
}

void RenderItem::cleanup()
{
	SwapContext set_ctx(ctx.get(), window());
	input_polygon.destroy();
}
