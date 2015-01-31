#pragma once

#include <getscene.h>
#include <QQuickItem>
#include <QQuickWindow>
#include <QOpenGLContext>

#include <memory>

class RenderItem : public QQuickItem
{
    Q_OBJECT
public:
    RenderItem();
public slots :
	void sync();
	void cleanup();
    void paint();
private slots :
	void handleWindowChanged(QQuickWindow *win);
private:
	TestScene &renderer;
	std::unique_ptr<QOpenGLContext> ctx;
};