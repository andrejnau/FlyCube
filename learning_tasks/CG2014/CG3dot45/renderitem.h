#pragma once

#include <print_image.h>
#include <print_polygon.h>
#include <QQuickItem>
#include <QQuickWindow>
#include <QString>
#include <QPixmap>
#include <QColor>
#include <QPainter>
#include <QOpenGLContext>
#include <memory>

enum class app_mode
{
    draw_image,
    enter_polygon
};

class RenderItem : public QQuickItem
{
    Q_OBJECT
public:
    RenderItem();
signals:

public slots:
    void paint();
    void paint_image();
    void paint_polygon();
    void handleWindowChanged(QQuickWindow *win);
	void sync();
	void cleanup();
public slots:
    void doSend_nextPoint(int, int);
    void doSend_Manual(bool);
    void doSend_Apply();
    void doSend_Point(int, int);
    int getSize();
    bool isClosed();
    bool isCorrectPolugon();
private:
    std::vector<glm::vec2> cur_point;
	std::unique_ptr<PrintImage> renderer;
	std::unique_ptr<PrintPolygon> input_polygon;
    app_mode cur_app_mode;
	QOpenGLContext *backgroundContext;
};