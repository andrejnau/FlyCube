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
    Q_PROPERTY(QString val_n1 READ getN1 WRITE setN1 NOTIFY changedN1)
    Q_PROPERTY(QString val_n2 READ getN2 WRITE setN2 NOTIFY changedN2)
public:
    RenderItem();
    int doGetTypeAngle(int);
    bool isInput() const;
    QString getN1() const;
    QString getN2() const;
signals:
    void inputChanged(bool);
    void setLabel(QString str);
    void changedN1(QString arg);
    void changedN2(QString arg);
public slots:
    void doSendPoint(int, int);
    void doSendFuturePoint(int, int);
    void doButton();
    void setInput(bool);
    void setN1(QString arg);
    void setN2(QString arg);
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
	int iteration(int cnt, float n1, float n2, glm::vec2 c, glm::vec2 d, std::vector<line> & output, glm::vec2& out_a, glm::vec2& out_b);
    std::pair<int, glm::vec2> cross_with_box(glm::vec2, glm::vec2);
private:
    bool m_inputRay;
    bool m_input;
    std::vector<glm::vec2> m_points;
    std::vector<glm::vec2> m_box;
    PrintPolygon &input_polygon;
    std::unique_ptr<QOpenGLContext> ctx;
    QString m_val_n1;
    QString m_val_n2;
};
