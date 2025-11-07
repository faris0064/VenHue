#include "include/controller.h"
#include "include/logger.h"
#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QTimer>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    // For QSettings
    QCoreApplication::setOrganizationName("VenHue");
    QCoreApplication::setApplicationName("VenHue");

    QQmlApplicationEngine engine;
    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);
    engine.loadFromModule("VenHue", "Main");

    Controller controller;
    engine.rootContext()->setContextProperty("controller", &controller);

    QTimer::singleShot(0, &controller, SLOT(startCueMonitor()));
    Logger::clear();

    return app.exec();
}
