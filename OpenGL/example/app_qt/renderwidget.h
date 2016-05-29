#pragma once

#include <testscene.h>
#include <QOpenGLWidget>
#include <memory>

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
    std::unique_ptr<TestScene> renderer;
};