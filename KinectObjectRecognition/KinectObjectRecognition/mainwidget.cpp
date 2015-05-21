#include "mainwidget.h"

/***********************************************************/
/*   Constructeur                                          */
/***********************************************************/
MainWidget::MainWidget(QWidget *parent)
	: QWidget(parent)
{
	this->showMaximized();
	
	// --- Création des éléments Qt ---------------------------
	_lblKinectColor = new QLabel();
	_lblKinectDepth = new QLabel();
	_btnScreenshotNeutre = new QPushButton("1. Screenshot Neutre");
	_btnScreenshotObjet = new QPushButton("2. Screenshot Objet");
	_btnDiff = new QPushButton("3. Difference des images");
	_btnRetry = new QPushButton("Recommencer");
	_btnQuitter = new QPushButton("Quitter");
	_txtStatut = new QPlainTextEdit();
	_timer = new QTimer(this);

	// --- Paramétrage des éléments Qt ------------------------
	_btnScreenshotNeutre->adjustSize();
	_btnScreenshotObjet->adjustSize();
	_btnScreenshotObjet->setDisabled(true);
	_btnDiff->adjustSize();
	_btnDiff->setDisabled(true);
	_btnQuitter->adjustSize();
	_btnQuitter->adjustSize();
	_txtStatut->setReadOnly(true);
	_txtStatut->setFont(QFont("Helvetica", 8));
	_txtStatut->setFixedHeight(100);

	// --- Création du layout horizontal de boutons------------
	QHBoxLayout* hLayout = new QHBoxLayout();
	hLayout->addWidget(_btnScreenshotNeutre);
	hLayout->addWidget(_btnScreenshotObjet);
	hLayout->addWidget(_btnDiff);
	hLayout->addWidget(_btnRetry);
	hLayout->addWidget(_btnQuitter);

	// --- Création du layout principal -----------------------
	QGridLayout* gridPrincipal = new QGridLayout(this);
	gridPrincipal->addWidget(_lblKinectColor, 0, 0, 1, 3);
	gridPrincipal->addWidget(_lblKinectDepth, 0, 3, 1, 3);
	gridPrincipal->addLayout(hLayout, 1, 1, 1, 4);
	gridPrincipal->addWidget(_txtStatut, 2, 0, 1, 6);

	// --- Visualisation de la fenêtre ------------------------
	setLayout(gridPrincipal);

	if (!InitKinect())
		_txtStatut->setPlainText("Kinect not found !");
	else
		_txtStatut->setPlainText("Kinect found !");

	// --- Connections ----------------------------------------
	connect(_timer, SIGNAL(timeout()), this, SLOT(processColor()));
	connect(_timer, SIGNAL(timeout()), this, SLOT(processDepth()));
	connect(_btnScreenshotNeutre, SIGNAL(clicked()), this, SLOT(cloneMatNeutre()));
	connect(_btnScreenshotObjet, SIGNAL(clicked()), this, SLOT(cloneMatObjet()));
	connect(_btnDiff, SIGNAL(clicked()), this, SLOT(difference()));
	connect(_btnRetry, SIGNAL(clicked()), this, SLOT(retry()));
	connect(_btnQuitter, SIGNAL(clicked()), qApp, SLOT(quit()));


	_timer->start(20);
}

/***********************************************************/
/*   Destructeur                                           */
/***********************************************************/
MainWidget::~MainWidget()
{
	// --- Destruction des éléments Qt --------------------------
	delete _lblKinectColor, _lblKinectDepth, _btnScreenshotNeutre, _btnQuitter, _txtStatut;

	// --- Destruction des éléments Kinect ----------------------
	NuiShutdown();

	// --- Fermeture des HANDLE ---------------------------------
	if (_nextFrameEventColor && (_nextFrameEventColor != INVALID_HANDLE_VALUE))
	{
		CloseHandle(_nextFrameEventColor);
		_nextFrameEventColor = NULL;
	}
	if (_nextFrameEventDepth && (_nextFrameEventDepth != INVALID_HANDLE_VALUE))
	{
		CloseHandle(_nextFrameEventDepth);
		_nextFrameEventDepth = NULL;
	}
}

/***********************************************************/
/*  Initialisation de la Kinect                            */
/***********************************************************/
bool MainWidget::InitKinect()
{
	// Initialisation de la Kinect et spécification d'utilisation de la couleur et de la profondeur
	if (FAILED(NuiInitialize(NUI_INITIALIZE_FLAG_USES_COLOR | NUI_INITIALIZE_FLAG_USES_DEPTH))) 
		return false;

	// Création d'évènements qui signaleront lorsque les données de couleur et de profondeur sont disponibles
	_nextFrameEventColor = CreateEvent(NULL, TRUE, FALSE, NULL);
	_nextFrameEventDepth = CreateEvent(NULL, TRUE, FALSE, NULL);

	// Ouverture d'un flux d'images pour recevoir les trames de couleur
	if (FAILED(NuiImageStreamOpen(NUI_IMAGE_TYPE_COLOR, 
									NUI_IMAGE_RESOLUTION_640x480,
									0,
									2,
									_nextFrameEventColor,
									&_streamHandleColor))) 
		return false;

	// Ouverture d'un flux d'images pour recevoir les trames de profondeur
	if (FAILED(NuiImageStreamOpen(NUI_IMAGE_TYPE_DEPTH,
									NUI_IMAGE_RESOLUTION_640x480,
									0,
									2,
									_nextFrameEventDepth,
									&_streamHandleDepth)))
		return false;

	// Si tout fonctionne, on peut créer notre matrice image couleur
	_imageColor = Mat(Size(WIDTH, HEIGHT), CV_8UC4);
	_imageDepth = Mat(Size(WIDTH, HEIGHT), CV_8UC1);
	return true;
}

/***********************************************************/
/*   Récupération des données de couleur                   */
/***********************************************************/
void MainWidget::processColor()
{
	const NUI_IMAGE_FRAME *frameColor = NULL;
	NUI_LOCKED_RECT lockedRectColor;

	if (FAILED(NuiImageStreamGetNextFrame(_streamHandleColor, 30, &frameColor)))
		return;

	frameColor->pFrameTexture->LockRect(0, &lockedRectColor, NULL, 0);

	if (lockedRectColor.Pitch != 0) 
	{
		_imageColor = Mat(HEIGHT, WIDTH, CV_8UC4, lockedRectColor.pBits, lockedRectColor.Pitch);
		
		// Conversion de l'image de BGRA (4 chaînes) en RGB (3 chaînes) 
		cvtColor(_imageColor, _imageColor, CV_BGRA2RGB);

		// Affichage de l'image couleur
		_lblKinectColor->setPixmap(QPixmap::fromImage(QImage(_imageColor.data,
			_imageColor.cols,
			_imageColor.rows,
			_imageColor.step,
			QImage::Format_RGB888)));
	}

	frameColor->pFrameTexture->UnlockRect(0);
	NuiImageStreamReleaseFrame(_streamHandleColor, frameColor);
}

/***********************************************************/
/*   Récupération des données de profondeur                */
/***********************************************************/
void MainWidget::processDepth()
{
	const NUI_IMAGE_FRAME *frameDepth = NULL;

	if (FAILED(NuiImageStreamGetNextFrame(_streamHandleDepth, 30, &frameDepth)))
		return;

	NUI_LOCKED_RECT lockedRectDepth;
	frameDepth->pFrameTexture->LockRect(0, &lockedRectDepth, NULL, 0);

	if (lockedRectDepth.Pitch != 0) 
	{
		USHORT *buffDepth = (USHORT*)lockedRectDepth.pBits;

		BYTE realDepth[WIDTH*HEIGHT];
		for (int i = 0; i < WIDTH * HEIGHT; i++)
		{
			realDepth[i] = NuiDepthPixelToDepth(buffDepth[i]) / 16;
		}

		// Création de la matrice de profondeur
		_imageDepth = Mat(HEIGHT, WIDTH, CV_8UC1, realDepth);

		// Affichage de l'image en niveau de gris sur 3 canaux
			_lblKinectDepth->setPixmap(QPixmap::fromImage(QImage(_imageDepth.data,
		_imageDepth.cols,
		_imageDepth.rows,
		_imageDepth.step,
		QImage::Format_Indexed8)));		
	}

	frameDepth->pFrameTexture->UnlockRect(0);
	NuiImageStreamReleaseFrame(_streamHandleDepth, frameDepth);
}

/***********************************************************/
/*   Récupération de la profondeur réelle                  */
/***********************************************************/
void MainWidget::getDepth(Mat src, Mat dest)
{
	for (int i = 0; i < src.cols; i++)
	{
		for (int j = 0; j < src.rows; j++)
		{
			dest.at<unsigned char>(i, j) = src.at<unsigned char>(i, j) * 16;
		}
	}
}

/***********************************************************/
/*   Récupération d'une image de profondeur sans l'objet   */
/***********************************************************/
void MainWidget::cloneMatNeutre()
{
	_backgroundDepth = _imageDepth.clone();

	disconnect(_timer, SIGNAL(timeout()), this, SLOT(processColor()));

	_btnScreenshotNeutre->setDisabled(true);
	_btnScreenshotObjet->setDisabled(false);

	_lblKinectColor->setPixmap(QPixmap::fromImage(QImage(_backgroundDepth.data,
		_backgroundDepth.cols,
		_backgroundDepth.rows,
		_backgroundDepth.step,
		QImage::Format_Indexed8)));
}

/***********************************************************/
/*   Récupération d'une image de profondeur avec l'objet   */
/***********************************************************/
void MainWidget::cloneMatObjet()
{
	_backAndObjectDepth = _imageDepth.clone();

	_btnScreenshotObjet->setDisabled(true);
	_btnDiff->setDisabled(false);

	_lblKinectColor->setPixmap(QPixmap::fromImage(QImage(_backAndObjectDepth.data,
															_backAndObjectDepth.cols,
															_backAndObjectDepth.rows,
															_backAndObjectDepth.step,
															QImage::Format_Indexed8)));
}

/***********************************************************/
/*   Récupération entre les images avec et sans l'objet    */
/***********************************************************/
void MainWidget::difference()
{
	Mat _objectDepth = Mat(_imageDepth.rows, _imageDepth.cols, CV_8UC1);
	absdiff(_backgroundDepth, _backAndObjectDepth, _objectDepth);
	Mat thresholdImage;
	threshold(_objectDepth, thresholdImage, 20, 255, THRESH_BINARY);

	_btnDiff->setDisabled(true);

	_lblKinectColor->setPixmap(QPixmap::fromImage(QImage(thresholdImage.data,
		thresholdImage.cols,
		thresholdImage.rows,
		thresholdImage.step,
		QImage::Format_Indexed8)));
}

/***********************************************************/
/*   Recommencer                                           */
/***********************************************************/
void MainWidget::retry()
{
	_btnScreenshotNeutre->setDisabled(false);
	_btnScreenshotObjet->setDisabled(true);
	_btnDiff->setDisabled(true);
	connect(_timer, SIGNAL(timeout()), this, SLOT(processColor()));
	connect(_timer, SIGNAL(timeout()), this, SLOT(processDepth()));
}
