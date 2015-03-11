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
    int doGetTypeAngle(int);
    bool isInput() const;
signals:
    void inputChanged(bool);
    void drawSortType(int drawX, int drawY, int drawType);
public slots:
    void doSendPoint(int, int);
    void doEndInput();
    void doSendFuturePoint(int, int);
    void setInput(bool);
private slots:
    void sync();
    void cleanup();
    void paint();
    void handleWindowChanged(QQuickWindow *);
private:
    std::pair<float, float> pointToGL(int, int);
    void paintPolygon();
    void cleanScene();
    void startTest(bool = true);
private:
    bool m_input;
    std::vector<std::pair<int, int> > point_for_sort;
    std::vector<int> sort_type;
    PrintPolygon &input_polygon;
    std::unique_ptr<QOpenGLContext> ctx;
};
