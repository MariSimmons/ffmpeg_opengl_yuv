#ifndef DECODER_H
#define DECODER_H

#include <QObject>
#include <QRunnable>

class OpenGLDisplayYUV;
class DecoderPrivate;

class Decoder : public QObject, public QRunnable
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(Decoder)
public:
    explicit Decoder(OpenGLDisplayYUV* display, QObject *parent = 0);
    ~Decoder() override;

    bool openRTSP(const QString& rtspUrl);

    void run() override;

public slots:
    void stop();

signals:
    void finished();

private:
    QScopedPointer<DecoderPrivate> d_ptr;
};

#endif // DECODER_H
