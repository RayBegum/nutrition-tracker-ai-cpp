// Nutrition Goal Tracker - version 1 (C++ / Qt + Claude)
#include "MainWindow.h"
#include <QApplication>

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    QApplication::setOrganizationName("NutritionTracker");
    QApplication::setApplicationName("NutritionTrackerAI");

    MainWindow window;
    window.show();
    return app.exec();
}
