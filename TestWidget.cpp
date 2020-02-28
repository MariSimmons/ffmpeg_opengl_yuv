#include "TestWidget.h"
#include "ui_TestWidget.h"

#include <QApplication>
#include <QFile>
#include <QByteArray>
#include <QDebug>
#include <QThread>
#include <QList>
#include <QWheelEvent>
#include <QThreadPool>
#include <QDebug>

#include "OpenGLDisplayYUV.h"
#include "Decoder.h"


#include <QtCore/QCoreApplication>
#include <QDebug>

struct TestWidget::TestWidgetImpl
{
    TestWidgetImpl()
        : ui(new Ui::TestWidget)
    {}

    Ui::TestWidget*             ui;
    unsigned char*              mBuffer;
    unsigned                    mSize;

    QList<OpenGLDisplayYUV*>       mPlayers;
};

TestWidget::TestWidget(QWidget *parent)
    : QWidget(parent)
    , impl(new TestWidgetImpl)
{
    QApplication::setAttribute(Qt::AA_ForceRasterWidgets, false);
//    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
    QCoreApplication::setAttribute(Qt::AA_UseDesktopOpenGL);

    impl->ui->setupUi(this);
    impl->ui->playAllButton->setEnabled(true);

    QGridLayout* wallLayout = new QGridLayout;
    for (int i = 0; i < 2; ++i)
    {
        for (int j = 0; j < 2; ++j)
        {
            auto player = new OpenGLDisplayYUV(this);
            wallLayout->addWidget(player, i, j);
            impl->mPlayers.append(player);
        }
    }
    wallLayout->setSpacing(2);
    wallLayout->setMargin(0);

    impl->ui->videoWall->setLayout(wallLayout);
    connect(impl->ui->playAllButton, &QPushButton::clicked, this, &TestWidget::playVideoWall);
}

TestWidget::~TestWidget()
{
    delete[] impl->mBuffer;
}

void TestWidget::playVideoWall()
{
    impl->ui->playAllButton->setEnabled(false);

    QThreadPool::globalInstance()->setMaxThreadCount(10);

    qDebug() << ">> Players count -" << impl->mPlayers.count();

    for (int i = 0; i < impl->mPlayers.count(); ++i)
    {
        Decoder* decoder = new Decoder(impl->mPlayers[i], this);
        if (!decoder->openRTSP("rtsp://freja.hiof.no:1935/rtplive/definst/hessdalen03.stream"))
            qWarning() << "Can't open rtsp";

        QObject::connect(impl->mPlayers[i], &OpenGLDisplayYUV::closed,decoder, &Decoder::stop);
        QObject::connect(decoder, &Decoder::finished, impl->mPlayers[i], &OpenGLDisplayYUV::deleteLater);
        QObject::connect(decoder, &Decoder::finished, this, &QApplication::quit);
        QThreadPool::globalInstance()->start(decoder);
    }
}
