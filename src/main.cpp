#include "include/controller.h"
#include "include/hue_connection_state.h"
#include "include/logger.h"
#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QTimer>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    Logger::clear();

    // For QSettings
    QCoreApplication::setOrganizationName("VenHue");
    QCoreApplication::setApplicationName("VenHue");

    qmlRegisterUncreatableType<HueConnectionState>("VenHue", 1, 0, "HueConnectionState",
                                                   "HueConnectionState is exposed by Controller");

    QQmlApplicationEngine engine;
    Controller controller;
    engine.rootContext()->setContextProperty("controller", &controller);
    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);
    engine.loadFromModule("VenHue", "Main");

    QTimer::singleShot(0, &controller, &Controller::initializeHue);
    QTimer::singleShot(0, &controller, SLOT(startCueMonitor()));
    return app.exec();
}
