#include "cvcamera.h"

#include <QGuiApplication>
#include <QAbstractVideoSurface>
#include <QImage>
#include <QDateTime>
#include <QTimer>

CvCamera::CvCamera(QObject *parent)
    : QObject{ parent }, m_Surface{ Q_NULLPTR }, m_AspectRatio{ 1920, 1024 }, m_FormatSet{ false }, m_Running{ false }
{
    QObject::connect(this, &CvCamera::newVideoContent, this, &CvCamera::onNewVideoContentReceived);

    start();
}

CvCamera::~CvCamera()
{
    stop();
}

QAbstractVideoSurface *CvCamera::videoSurface() const
{
    return m_Surface;
}

void CvCamera::setVideoSurface(QAbstractVideoSurface *surface)
{
    if (m_Surface && m_Surface != surface  && m_Surface->isActive())
        m_Surface->stop();

    m_Surface = surface;

    if (m_Surface && m_Format.isValid()) {
        m_Format = m_Surface->nearestFormat(m_Format);
        m_Surface->start(m_Format);
    }
}

void CvCamera::setAspectRatio(int w, int h)
{
    m_AspectRatio = QSize(w, h);
}

void CvCamera::start()
{
    open();

    m_Running = true;
    if(!m_FutureGrabVideoContents.isRunning())
        m_FutureGrabVideoContents = QtConcurrent::run(this, &CvCamera::grabVideoContents);
}

void CvCamera::stop()
{
    m_Running = false;
    if(m_FutureGrabVideoContents.isRunning())
        m_FutureGrabVideoContents.waitForFinished();

    release();
}

void CvCamera::onNewVideoContentReceived(const QVideoFrame &frame)
{
    if (m_Surface)
        m_Surface->present(frame);
}

void CvCamera::open()
{
    if(!m_Cam.isOpened()) {
        m_Cam.open("*****");

        m_AspectRatio = QSize(m_Cam.get(CV_CAP_PROP_FRAME_WIDTH), m_Cam.get(CV_CAP_PROP_FRAME_HEIGHT));
    }

    if(!m_Cam.isOpened())
        QGuiApplication::exit(EXIT_FAILURE);
}

void CvCamera::release()
{
    if(m_Cam.isOpened())
        m_Cam.release();
}

void CvCamera::setFormat(const int width, const int heigth, const QVideoFrame::PixelFormat& format)
{
    m_Format = QVideoSurfaceFormat(QSize(width, heigth), format);

    if (m_Surface)
    {
        if (m_Surface->isActive())
        {
            m_Surface->stop();
        }
        m_Format = m_Surface->nearestFormat(m_Format);
        m_Surface->start(m_Format);
    }
}

void CvCamera::grabVideoContents()
{
    //QDateTime dateTime = QDateTime::currentDateTime();
    //cv::VideoWriter videoWriter(VIDEO_PATH + dateTime.toString().toStdString() + ".avi", CV_FOURCC('P','I','M','1'), 20, cv::Size(m_AspectRatio.width(), m_AspectRatio.height()), true);

    while(true) {

        //QDateTime currDateTime = QDateTime::currentDateTime();
        //if((currDateTime.toSecsSinceEpoch() - dateTime.toSecsSinceEpoch()) >= 3600) {
        //    videoWriter = cv::VideoWriter(VIDEO_PATH + dateTime.toString().toStdString() + ".avi", CV_FOURCC('P','I','M','1'), 20, cv::Size(m_AspectRatio.width(), m_AspectRatio.height()), true);
        //    dateTime = currDateTime;
        //}

        m_Mat.release();
        m_Cam.read(m_Mat);

        cv::cvtColor(m_Mat, m_MatRGB, cv::COLOR_BGR2RGB);

        QImage img(m_MatRGB.data, m_MatRGB.cols, m_MatRGB.rows, static_cast<int>(m_MatRGB.step), QImage::Format_RGB888);
        img = img.convertToFormat(QImage::Format_RGB32).scaled(m_AspectRatio);

        QVideoFrame frame(img);
        if(!m_FormatSet) {
            setFormat(frame.width(), frame.height(), frame.pixelFormat());
            m_FormatSet = true;
        }

        Q_EMIT newVideoContent(frame);

        //videoWriter.write(m_Mat);

        QThread::msleep(5);

        if(!m_Running) {
            return;
        }
    }
}
