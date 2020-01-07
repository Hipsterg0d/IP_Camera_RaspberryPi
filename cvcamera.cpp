#include "cvcamera.h"

#include <QGuiApplication>
#include <QAbstractVideoSurface>
#include <QImage>
#include <QDateTime>
#include <QTimer>

#include <QDebug>

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
    while(!m_Cam.isOpened()) {
        m_Cam.open("************");
    }

    m_AspectRatio = QSize(m_Cam.get(CV_CAP_PROP_FRAME_WIDTH), m_Cam.get(CV_CAP_PROP_FRAME_HEIGHT));
    m_NumPixels = m_AspectRatio.width() * m_AspectRatio.height();
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
    cv::VideoWriter videoWriter;

    m_Cam.read(m_Mat);
    m_Cam.read(m_NewMat);

    cv::Mat matGray(m_Mat.size(), CV_8U), newMatGray(m_NewMat.size(), CV_8U);
    cv::Mat diff(m_Mat.size(), CV_8U);
    std::vector<std::vector<cv::Point>> contours;

    qint64 movementTime;
    bool saveVideo = false;
    m_lastMovement = 0;

    while(true) {

        if(!m_Cam.isOpened())
            open();

        if((movementTime = detectMotion(matGray, newMatGray, diff, contours)) > 0)
            m_lastMovement = movementTime;

        QDateTime dateTime = QDateTime::currentDateTime();
        if((dateTime.toSecsSinceEpoch() - m_lastMovement) < 30) {
            if(saveVideo)
                videoWriter.write(m_Mat);
            else {
                saveVideo = true;
                videoWriter = cv::VideoWriter(VIDEO_PATH + dateTime.toString().toStdString() + ".avi", CV_FOURCC('P','I','M','1'), 20, cv::Size(m_AspectRatio.width(), m_AspectRatio.height()), true);
                videoWriter.write(m_Mat);
            }
        }
        else
            saveVideo = false;

        cv::cvtColor(m_Mat, m_MatRGB, cv::COLOR_BGR2RGB);

        QImage img(m_MatRGB.data, m_MatRGB.cols, m_MatRGB.rows, static_cast<int>(m_MatRGB.step), QImage::Format_RGB888);
        img = img.convertToFormat(QImage::Format_RGB32).scaled(m_AspectRatio);

        QVideoFrame frame(img);
        if(!m_FormatSet) {
            setFormat(frame.width(), frame.height(), frame.pixelFormat());
            m_FormatSet = true;
        }

        Q_EMIT newVideoContent(frame);

        m_NewMat.copyTo(m_Mat);
        m_NewMat.release();
        m_Cam.read(m_NewMat);

        QThread::msleep(5);

        if(!m_Running) {
            return;
        }
    }
}

qint64 CvCamera::detectMotion(cv::Mat &matGray, cv::Mat &newMatGray, cv::Mat &diff, std::vector<std::vector<cv::Point>>& contours)
{
    cv::cvtColor(m_Mat, matGray, cv::COLOR_BGR2GRAY);
    cv::cvtColor(m_NewMat, newMatGray, cv::COLOR_BGR2GRAY);

    cv::absdiff(matGray, newMatGray, diff);
    cv::GaussianBlur(diff, diff, cv::Size(5, 5), 0);
    cv::threshold(diff, diff, 20, 255, cv::THRESH_BINARY);
    cv::dilate(diff, diff, 0, cv::Point(-1, -1), 3);

    cv::findContours(diff, contours, cv::RETR_TREE, cv::CHAIN_APPROX_SIMPLE);

    for (int i = 0; i < contours.size(); i++) {
        cv::Rect rec = cv::boundingRect(contours[i]);
        cv::rectangle(m_Mat, rec, cv::Scalar(0, 255, 0), 2);
    }

    int num = 0;
    for (int i = 0; i < m_AspectRatio.height(); i++)
        for (int j = 0; j < m_AspectRatio.width(); j++)
            if (diff.at<unsigned char>(i, j) == 255)
                num++;
    double avg = (num * 100.0) / m_NumPixels;
    if (avg >= 0.1)
        return QDateTime::currentDateTime().toSecsSinceEpoch();
    else
        return 0;
}
