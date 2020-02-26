#pragma once

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QScopedPointer>
#include <QException>

QT_FORWARD_DECLARE_CLASS(QCloseEvent)

class OpenGLDisplayYUV : public QOpenGLWidget, public QOpenGLFunctions
{
    Q_OBJECT
public:
    explicit OpenGLDisplayYUV(QWidget* parent = nullptr);
    ~OpenGLDisplayYUV() override;

    void DisplayVideoFrame(unsigned char **data, int frameWidth, int frameHeight);

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;

    void closeEvent(QCloseEvent* e) override;

signals:
    Q_SIGNAL void closed();

private:
    struct OpenGLDisplayYUVImpl;
    QScopedPointer<OpenGLDisplayYUVImpl> impl;
};
