#pragma once

#include <print_polygon.h>

#include <QQuickItem>
#include <QQuickWindow>
#include <QOpenGLContext>

#include <memory>

class RenderItem : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(bool input READ isInput WRITE setInput NOTIFY inputChanged)
public:
    RenderItem();
    bool isInput() const;
    void setInput(bool);
signals:
    void inputChanged(bool);
public slots:
    void doSendPoint(int, int);
    void doSendFuturePoint(int, int);
private slots:
    void sync();
    void cleanup();
    void paint();
    void handleWindowChanged(QQuickWindow *);
private:
    std::pair<float, float> pointToGL(int, int);
    void paintPolygon();
    void cleanScene();
private:
    bool m_input;
    PrintPolygon &input_polygon;
    std::unique_ptr<QOpenGLContext> ctx;
};
