#pragma once

#include <print_polygon.h>

#include <QQuickItem>
#include <QQuickWindow>
#include <QOpenGLContext>
#include <QString>
#include <assert.h>

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
    void setLabel(QString str);
public slots:
    void doSendPoint(int, int);
    void doSendFuturePoint(int, int);
    void doButton();
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
    void finishBox();
    void Birefringence();
    std::pair<int, glm::vec2> cross_with_box(glm::vec2, glm::vec2);
private:
    bool m_inputRay;
    bool m_input;
    std::vector<glm::vec2> m_points;
    std::vector<glm::vec2> m_box;
    PrintPolygon &input_polygon;
    std::unique_ptr<QOpenGLContext> ctx;
};
