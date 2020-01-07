#ifndef CVCAMERA_H
#define CVCAMERA_H

#include "opencv2/opencv.hpp"

#include <QObject>
#include <QVideoSurfaceFormat>
#include <QVideoFrame>
#include <QPair>
#include <QSize>
#include <QtConcurrent/QtConcurrent>
#include <memory>

QT_FORWARD_DECLARE_CLASS(QAbstractVideoSurface)

#ifdef Q_PROCESSOR_ARM
    #define VIDEO_PATH "/home/root/videos/"
#else
    #define VIDEO_PATH "/home/david/Schreibtisch/video/"
#endif

class CvCamera : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QAbstractVideoSurface* videoSurface READ videoSurface WRITE setVideoSurface)

public:
    explicit CvCamera(QObject* parent = Q_NULLPTR);
    ~CvCamera();

    QAbstractVideoSurface* videoSurface() const;
    void setVideoSurface(QAbstractVideoSurface* surface);

    Q_INVOKABLE void setAspectRatio(int w, int h);
    Q_INVOKABLE void start();
    Q_INVOKABLE void stop();

Q_SIGNALS:
    void newVideoContent(const QVideoFrame&);

public Q_SLOTS:
    void onNewVideoContentReceived(const QVideoFrame& frame);

private:
    void open();
    void release();
    void setFormat(const int width, const int heigth, const QVideoFrame::PixelFormat& format);
    void grabVideoContents();
    void newVideoFile();

    QAbstractVideoSurface* m_Surface;
    QVideoSurfaceFormat m_Format;
    QFuture<void> m_FutureGrabVideoContents;
    QSize m_AspectRatio;
    cv::VideoCapture m_Cam;

    cv::Mat m_Mat;
    cv::Mat m_MatRGB;
    bool m_FormatSet;
    bool m_Running;
};

#endif // CVCAMERA_H
