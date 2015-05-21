#ifndef MAINWIDGET_H
#define MAINWIDGET_H

// Qt
#include <QtWidgets/QWidget>
#include <QtWidgets\QLabel.h>
#include <QtWidgets\QPushButton.h>
#include <QtWidgets\QPlainTextEdit.h>
#include <QtWidgets\QGridLayout.h>
#include <QtWidgets\QBoxLayout.h>
#include <QtWidgets\QSlider.h>
#include <QtWidgets\QApplication.h>
#include <QtCore\QTimer.h>

// Kinect SDK
#include <Windows.h>
#include <NuiApi.h>


// OpenCV
#include <opencv2\opencv.hpp>
#include <opencv2\imgproc\imgproc.hpp>

// Namespace
using namespace std;
using namespace cv;

// Define
#define WIDTH				640
#define HEIGHT				480

class MainWidget : public QWidget
{
	Q_OBJECT

public:
	MainWidget(QWidget *parent = 0);
	~MainWidget();

	// --- Kinect -----------------------------------------
	bool InitKinect();

	// --- OpenCV ----------------------------------------
	void getDepth(Mat src, Mat dest);	

	public slots:
		void processColor(); // Récupère une trame RGBA (32 bits) de la Kinect
		void processDepth(); // Récupère une trame 16 bits de la Kinect
		void cloneMatNeutre();
		void cloneMatObjet();
		void difference();
		void retry();

private:
	// --- Qt ----------------------------------------------
	QLabel *_lblKinectColor, *_lblKinectDepth;
	QPushButton *_btnScreenshotNeutre, *_btnScreenshotObjet, *_btnDiff, *_btnRetry, *_btnQuitter;
	QPlainTextEdit *_txtStatut;
	QTimer *_timer;

	// --- Kinect ------------------------------------------
	HANDLE _nextFrameEventColor, _streamHandleColor;
	HANDLE _nextFrameEventDepth, _streamHandleDepth;

	// --- OpenCV ------------------------------------------
	Mat _imageColor, _imageDepth, _nextImageDepth;
	Mat _matDistanceDepth;
	Mat _backgroundDepth, _backAndObjectDepth;
};

#endif // MAINWIDGET_H
