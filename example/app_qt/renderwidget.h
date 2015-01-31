#pragma once
#include <getscene.h>
#include <QOpenGLWidget>

class RenderWidget : public QOpenGLWidget
{
    Q_OBJECT
public:
	explicit RenderWidget(QWidget *parent = 0);
	~RenderWidget();
protected:
    void initializeGL();
    void resizeGL(int nWidth, int nHeight);
    void paintGL();
private slots:
	void cleanup();
private:
    SceneBase &renderer;
};