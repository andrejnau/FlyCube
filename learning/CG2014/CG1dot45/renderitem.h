#pragma once

#include <fractal.h>
#include <print_polygon.h>
#include <QQuickItem>
#include <QQuickWindow>
#include <QString>
#include <QOpenGLContext>
#include <memory>

enum class app_mode
{
	draw_fractal,
	enter_polygon
};

class RenderItem : public QQuickItem
{
	Q_OBJECT
public:
	RenderItem();
signals:

	public slots :
		void paint();
	void paint_fractal();
	void paint_polygon();
	void handleWindowChanged(QQuickWindow *win);
	void sync();
	void cleanup();

	public slots:
	void doSend_Manual(bool);
	void doSend_Apply(int);
	void doSend_Point(int, int);
	void doSend_Vertex(int);
	void doSend_Eps(double);
	void doSend_Iteration(int);
	void doSend_D(int);
	void doSend_Di(int);
	void doSend_D0(int);

private:
	std::vector<glm::vec2> cur_point;
	std::unique_ptr<Fractal> renderer;
	std::unique_ptr<PrintPolygon> input_polygon;
	app_mode cur_app_mode;
};