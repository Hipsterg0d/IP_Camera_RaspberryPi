#include "cvcamera.h"

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QCursor>

int main(int argc, char *argv[])
{
    qputenv("QT_IM_MODULE", QByteArray("qtvirtualkeyboard"));

    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling); 

    QGuiApplication app(argc, argv);

    QGuiApplication::setOverrideCursor(QCursor(Qt::BlankCursor));

    qmlRegisterType<CvCamera>("com.ip_camera.cvcamera", 1, 0, "CvCamera");

    QQmlApplicationEngine engine;
    const QUrl url(QStringLiteral("qrc:/main.qml"));
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl)
            QCoreApplication::exit(-1);
    }, Qt::QueuedConnection);
    engine.load(url);

    return app.exec();
}
