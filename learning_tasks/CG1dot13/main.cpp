#include <renderitem.h>

#include <QApplication>
#include <QQmlApplicationEngine>
#include <QtQml>
#include <QQuickView>

int main(int argc, char *argv[])
{
	QApplication app(argc, argv);

	qmlRegisterType<RenderItem>("OpenGLUnderQML", 1, 0, "Squircle");
	QQuickView view;
	view.setResizeMode(QQuickView::SizeRootObjectToView);
	view.setSource(QUrl("qrc:///main.qml"));
	view.setTitle("Computer Graphics. Task 1.13");
	view.show();

	return app.exec();
}
