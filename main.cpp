#include <QApplication>
#include <QDebug>
#include <QThreadPool>
#include <QTimer>
#include <QDialog>
#include <QVBoxLayout>

#include "TestWidget.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    TestWidget suffering;
    suffering.show();

    return app.exec();
}

