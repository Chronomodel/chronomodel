#include "CalibrationView.h"
#include "Ruler.h"
#include "Date.h"
#include "Event.h"
#include "Marker.h"
#include "../PluginAbstract.h"
#include "../GraphViewRefAbstract.h"
#include "MainWindow.h"
#include "Project.h"
#include "GraphView.h"
#include "CheckBox.h"
#include "LineEdit.h"
#include "Button.h"
#include "StdUtilities.h"
#include "Painting.h"
#include "Label.h"
#include "ModelUtilities.h"
#include "QtUtilities.h"
#include "DoubleValidator.h"

#include "CalibrationDrawing.h"

#include <QtWidgets>
#include <QClipboard>
//#include <QStringBuilder>



CalibrationView::CalibrationView(QWidget* parent, Qt::WindowFlags flags):QWidget(parent, flags),
    mRefGraphView(nullptr),
    mResultsHeight(90),
    mButtonWidth(50),
    mTminDisplay(-INFINITY),
    mTmaxDisplay(INFINITY)
{

    mDrawing = new CalibrationDrawing(this);
    mDrawing->setMouseTracking(true);
    setMouseTracking(true);


    mImageSaveBut = new Button(tr("Save"), this);
    mImageSaveBut->setIcon(QIcon(":picture_save.png"));
    mImageSaveBut->setFlatVertical();
    mImageSaveBut->setToolTip(tr("Save image as file"));
    mImageSaveBut->setIconOnly(true);

    mImageClipBut = new Button(tr("Copy"), this);
    mImageClipBut->setIcon(QIcon(":picture_copy.png"));
    mImageClipBut->setFlatVertical();
    mImageClipBut->setToolTip(tr("Copy image to clipboard"));
    mImageClipBut->setIconOnly(true);

    mResultsClipBut = new Button(tr("Copy"), this);
    mResultsClipBut->setIcon(QIcon(":text.png"));
    mResultsClipBut->setFlatVertical();
    mResultsClipBut->setToolTip(tr("Copy text results to clipboard"));
    mResultsClipBut->setIconOnly(true);

    frameSeparator = new QFrame(this);
    frameSeparator->setFrameStyle(QFrame::Panel);
    frameSeparator->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
  //  frameSeparator->setStyleSheet("QFrame { border: 5px solid rgb(49, 112, 176); }");//same color as Painting::mainColorLight = QColor(49, 112, 176);


    mHPDLab = new Label(tr("HPD (%)"), this);
    mHPDLab->setAlignment(Qt::AlignHCenter);
    mHPDLab->setLight();

    mHPDEdit = new LineEdit(this);
    mHPDEdit->setText("95");

    DoubleValidator* percentValidator = new DoubleValidator();
    percentValidator->setBottom(0.);
    percentValidator->setTop(100.);
    percentValidator->setDecimals(3);
    mHPDEdit->setValidator(percentValidator);

    mStartLab = new Label(tr("Start"), this);
    mStartLab->setAlignment(Qt::AlignHCenter);
    mStartLab->setLight();

    mStartEdit = new LineEdit(this);
    mStartEdit->setText("-1000");

    mEndLab = new Label(tr("End"), this);
    mEndLab->setAlignment(Qt::AlignHCenter);
    mEndLab->setLight();

    mEndEdit = new LineEdit(this);
    mEndEdit->setText("1000");

    mHPDLab->raise();
    mHPDEdit->raise();

    //-------- DrawingView


    mCalibGraph = new GraphView(mDrawing);
    
    mCalibGraph->setRendering(GraphView::eHD);
    mCalibGraph->setYAxisMode(GraphView::eHidden);
    mCalibGraph->setXAxisMode(GraphView::eAllTicks);
    mCalibGraph->setMouseTracking(true);

    mResultsText = new QTextEdit(this);
    mResultsText->setFrameStyle(QFrame::HLine);
    QPalette palette = this->palette();
    palette.setColor(QPalette::Base, Qt::white);
    palette.setColor(QPalette::Text, Qt::black);

    mResultsText->setPalette(palette);
    mResultsText->setText(tr("No Result to display"));
    mResultsText->setReadOnly(true);
    

    // Connection
    // setText doesn't emit signal textEdited, when the text is changed programmatically

    connect(mStartEdit, static_cast<void (QLineEdit::*)(const QString&)>(&QLineEdit::textEdited), this, &CalibrationView::updateScroll);
    connect(mEndEdit, static_cast<void (QLineEdit::*)(const QString&)>(&QLineEdit::textEdited), this, &CalibrationView::updateScroll);
    connect(mHPDEdit, &QLineEdit::textEdited, this, &CalibrationView::updateGraphs);
    connect(mImageSaveBut, &Button::clicked, this, &CalibrationView::exportImage);
    connect(mResultsClipBut, &Button::clicked, this, &CalibrationView::copyText);
    connect(mImageClipBut, &Button::clicked, this, &CalibrationView::copyImage);


    setVisible(false);
}

CalibrationView::~CalibrationView()
{
    mRefGraphView = nullptr;

}

void CalibrationView::setFont(const QFont &font)
{
    // we must force setFont on QLineEdi !!
    mHPDEdit->setFont(font);
    mStartEdit->setFont(font);
    mEndEdit->setFont(font);
    repaint();
}

void CalibrationView::setDate(const QJsonObject& date)
{
    Q_ASSERT(&date);
    if (date.isEmpty())
        return;

    Project* project = MainWindow::getInstance()->getProject();
    const QJsonObject state = project->state();
    const QJsonObject settings = state.value(STATE_SETTINGS).toObject();
    mSettings = ProjectSettings::fromJson(settings);

    if (mRefGraphView) {
         if (mDate.mPlugin)
                    mDate.mPlugin->deleteGraphViewRef(mRefGraphView);
        mRefGraphView = nullptr;
    }

    try {
        mDate.init();
        mDate = Date::fromJson(date);
        mDate.autoSetTiSampler(false);

        mDrawing->setTitle(mDate.mName + " (" + mDate.mPlugin->getName() + ")");

        const double t1 ( mSettings.getTminFormated() );
        const double t2 ( mSettings.getTmaxFormated() );
        const double t3 ( mDate.getFormatedTminCalib() );
        const double t4 ( mDate.getFormatedTmaxCalib() );

        mTminDisplay = qMin(t1, t3);
        mTmaxDisplay = qMax(t2, t4);

        // setText doesn't emit signal textEdited, when the text is changed programmatically
        mStartEdit->setText(stringWithAppSettings(mTminDisplay, false));
        mEndEdit->setText(stringWithAppSettings(mTmaxDisplay, false));

        if (std::isinf(-mTminDisplay) || std::isinf(mTmaxDisplay))
            throw("CalibrationView::setDate "+mDate.mPlugin->getName()+ mDate.mCalibration->mName + mDate.mCalibration->mTmin+mDate.mCalibration->mTmax);

        updateScroll();
        //updateGraphs();
    }
    catch(QString error) {
        QMessageBox message(QMessageBox::Critical,
                            qApp->applicationName() + " " + qApp->applicationVersion(),
                            tr("Error : ") + error,
                            QMessageBox::Ok,
                            qApp->activeWindow(),
                            Qt::Sheet);
        message.exec();
    }
    mDrawing->updateLayout();
    update();
}

void CalibrationView::updateGraphs()
{
    mCalibGraph->removeAllCurves();
    mCalibGraph->removeAllZones();
    
    //const QLocale locale;
    

    if (!mDate.isNull()) {
qDebug()<<" CalibrationView::updateGraphs()"<<mTminDisplay<<mTmaxDisplay;
        mCalibGraph->setRangeX(mTminDisplay, mTmaxDisplay);
        mCalibGraph->setCurrentX(mTminDisplay, mTmaxDisplay);
        
        // ------------------------------------------------------------
        //  Show zones if calibrated data are outside study period
        // ------------------------------------------------------------
        if (mTminDisplay < mSettings.getTminFormated()) {
            GraphZone zone;
            zone.mXStart = -INFINITY;
            zone.mXEnd = mSettings.getTminFormated();
            zone.mColor = QColor(217, 163, 69);
            zone.mColor.setAlpha(35);
            zone.mText = tr("Outside study period");
            mCalibGraph->addZone(zone);
        }
        if (mTmaxDisplay > mSettings.getTmaxFormated()) {
            GraphZone zone;
            zone.mXStart = mSettings.getTmaxFormated();
            zone.mXEnd = INFINITY;
            zone.mColor = QColor(217, 163, 69);
            zone.mColor.setAlpha(35);
            zone.mText = tr("Outside study period");
            mCalibGraph->addZone(zone);
        }
        
        // ------------------------------------------------------------
        //  Calibration curve
        // ------------------------------------------------------------
        QColor penColor = Painting::mainColorDark;
        QColor brushColor = Painting::mainColorLight;
        brushColor.setAlpha(100);

        // Fill under distrib. of calibrated date only if typo :
        const bool isTypo (mDate.mPlugin->getName() == "Typo");
      /*  mHPDLab->setVisible(!isTypo);
        mHPDEdit->setVisible(!isTypo);*/

        // Fill HPD only if not typo :
        mResultsText->clear();
        QMap<double, double> calibMap = mDate.getFormatedCalibMap();

        if (!calibMap.isEmpty()) {
            // ------------------------------------------------------------
            //  Display numerical results
            // ------------------------------------------------------------
            QString resultsStr;
            
            DensityAnalysis results;
            results.analysis = analyseFunction(calibMap);

            results.quartiles = quartilesForRepartition(mDate.getFormatedRepartition(), mDate.getFormatedTminCalib(), mSettings.mStep);

            resultsStr = densityAnalysisToString(results);
            
            GraphCurve calibCurve;
            calibCurve.mName = "Calibration";
            calibCurve.mPen.setColor(penColor);
            calibCurve.mIsHisto = false;
            calibCurve.mData = calibMap;
            calibCurve.mIsRectFromZero = isTypo;
            calibCurve.mBrush = isTypo ? QBrush(brushColor) : QBrush(Qt::NoBrush);

            type_data yMax = map_max_value(calibCurve.mData);
            yMax = (yMax > 0.) ? yMax : 1.;
            mCalibGraph->setRangeY(0., 1. * yMax);

            mCalibGraph->addCurve(calibCurve);
           // mCalibGraph->setVisible(true);
            mCalibGraph->setMarginBottom(mCalibGraph->font().pointSizeF() + 10.);

         //   mCalibGraph->setTipXLab("t"); // don't work

            QString input = mHPDEdit->text();
            mHPDEdit->validator()->fixup(input);
            mHPDEdit->setText(input);
          //  QLocale loc = QLocale();

            const double thresh = qBound(0., locale().toDouble(input), 100.);

            QMap<type_data, type_data> hpd = create_HPD(calibCurve.mData, thresh);
            
            GraphCurve hpdCurve;
            hpdCurve.mName = "Calibration HPD";
            hpdCurve.mPen = penColor;
            hpdCurve.mBrush = brushColor;
            hpdCurve.mIsHisto = false;
            hpdCurve.mIsRectFromZero = true;
            hpdCurve.mData = hpd;
            mCalibGraph->addCurve(hpdCurve);
            
            yMax = map_max_value(hpdCurve.mData);

            mCalibGraph->setRangeY(0., 1. * yMax);

            mCalibGraph->mLegendX = DateUtils::getAppSettingsFormatStr();
            mCalibGraph->setFormatFunctX(stringWithAppSettings);
            mCalibGraph->setFormatFunctY(stringWithAppSettings);
            
            double realThresh = map_area(hpd) / map_area(calibCurve.mData);
            
            resultsStr += + "<br> HPD (" + locale().toString(100. * realThresh, 'f', 1) + "%) : " + getHPDText(hpd, realThresh * 100.,DateUtils::getAppSettingsFormatStr(), stringWithAppSettings);
            
            mResultsText->setText(resultsStr);
            mDrawing->setCalibGraph(mCalibGraph);
        }

        // ------------------------------------------------------------
        //  Reference curve from plugin
        // ------------------------------------------------------------
        

        // Get the ref graph for this plugin and this date
        if (!mRefGraphView) {

            mRefGraphView = mDate.mPlugin->getGraphViewRef();
        }

        if (mRefGraphView) {
            mRefGraphView->setParent(mDrawing);
            mRefGraphView->setVisible(true);

            mRefGraphView->setFormatFunctX(stringWithAppSettings); // must be before setDate, because setDate use it
            ProjectSettings tmpSettings;
            tmpSettings.mTmax = mTmaxDisplay;
            tmpSettings.mTmin = mTminDisplay;
            tmpSettings.mStep = 1.;

            mRefGraphView->setDate(mDate, tmpSettings);
            mRefGraphView->zoomX(mTminDisplay, mTmaxDisplay);

        }
        mDrawing->setRefGraph(mRefGraphView);
        mDrawing->updateLayout();
        // ------------------------------------------------------------
        //  Labels
        // ------------------------------------------------------------
        if (mRefGraphView) {
            mDrawing->setRefTitle(tr("Calibration process"));
            mDrawing->setRefComment(mDate.mPlugin->getDateDesc(&mDate));
            mDrawing->setCalibTitle(tr("Distribution of calibrated date"));
            mDrawing->setCalibComment("HPD = " + mHPDEdit->text() + " %");

        } else {
            mDrawing->setRefTitle(tr("No calibration process to display"));
            mDrawing->setRefComment(mDate.mPlugin->getDateDesc(&mDate));
            mDrawing->setCalibTitle(tr("Typological date"));
            mDrawing->setCalibComment("HPD = " + mHPDEdit->text() + " %");
        }
    }
    

    updateLayout();
}

void CalibrationView::updateZoom()
{
    type_data min = mCalibGraph->minimumX();
    type_data max = mCalibGraph->maximumX();
    type_data minProp = 5. / (max - min);
    type_data prop = 1;
    if (prop < minProp)
        prop = minProp;
    
    if (prop != 1) {
        // Remember old scroll position
        type_data posProp (0);
        type_data rangeBefore = (type_data) 10;
        if (rangeBefore > 0)
            posProp = 1;

        // Set scroll to correct position
        /*type_data pos (0.);
        type_data rangeAfter = (type_data) 10;
        if (rangeAfter > 0)
            pos = floor(posProp * rangeAfter);
        */
        
    } else
        updateScroll();

}

void CalibrationView::updateScroll()
{
    bool ok;
    double val = locale().toDouble(mStartEdit->text(),&ok);
    if (ok)
        mTminDisplay = val;
    else
        return;

    val = locale().toDouble(mEndEdit->text(),&ok);
    if (ok)
        mTmaxDisplay = val;
    else
        return;

    QFont adaptedFont (font());
    QFontMetricsF fm (font());
    qreal textSize = fm.width(mStartEdit->text());
    if (textSize > (mStartEdit->width() - 2. )) {
        const qreal fontRate = textSize / (mStartEdit->width() - 2. );
        const qreal ptSiz = adaptedFont.pointSizeF() / fontRate;
        adaptedFont.setPointSizeF(ptSiz);
        mStartEdit->setFont(adaptedFont);
    }
    else
        mStartEdit->setFont(font());

    adaptedFont = font();
    fm = QFontMetrics(font());
    textSize = fm.width(mEndEdit->text());
    if (textSize > (mEndEdit->width() - 2. )) {
        const qreal fontRate = textSize / (mEndEdit->width() - 2. );
        const qreal ptSiz = adaptedFont.pointSizeF() / fontRate;
        adaptedFont.setPointSizeF(ptSiz);
        mEndEdit->setFont(adaptedFont);
    }
    else
        mEndEdit->setFont(font());
qDebug()<<"CalibrationView::updateScroll()"<<mTminDisplay<<mTmaxDisplay;
    // usefull when we set mStartEdit and mEndEdit at the begin of the display,
    // after a call to setDate
    if (std::isinf(-mTminDisplay) || std::isinf(mTmaxDisplay))
        return;
    else if (mTminDisplay<mTmaxDisplay)
            updateGraphs();
    else
        return;

}

void CalibrationView::exportImage()
{
    mDrawing->hideMarker();

    QFileInfo fileInfo = saveWidgetAsImage(mDrawing, mDrawing->rect(), tr("Save calibration image as..."),
                                           MainWindow::getInstance()->getCurrentPath(), MainWindow::getInstance()->getAppSettings());
    if (fileInfo.isFile())
        MainWindow::getInstance()->setCurrentPath(fileInfo.dir().absolutePath());
    
    mDrawing->showMarker();

}
void CalibrationView::copyImage()
{
    mDrawing->hideMarker();
    repaint();
    QApplication::clipboard()->setPixmap(mDrawing->grab());
    mDrawing->showMarker();

}
void CalibrationView::copyText()
{
     //QClipboard *p_Clipboard = QApplication::clipboard();
    //str.remove(QRegExp("<[^>]*>"));
    //p_Clipboard->setText(mResultsText->text().simplified());
    /*QTextDocument doc;
    doc.setHtml( mResultsLab->text() );
    p_Clipboard->setText(doc.toPlainText());
     */
    QString text = mDate.mName + " (" + mDate.mPlugin->getName() + ")" +"<br>" + mDate.getDesc() + "<br>" + mResultsText->toPlainText();// ->text();
    QApplication::clipboard()->setText(text.replace("<br>", "\r"));
   
}

void CalibrationView::setVisible(bool visible)
{
    mImageSaveBut->setVisible(visible);
    mImageClipBut->setVisible(visible);
    mResultsClipBut->setVisible(visible);
    frameSeparator->setVisible(visible);

    mStartLab->setVisible(visible);
    mStartEdit->setVisible(visible);
    mEndLab->setVisible(visible);
    mEndEdit->setVisible(visible);
    // Fill under distrib. of calibrated date only if typo :
    const bool isTypo (mDate.mPlugin && (mDate.mPlugin->getName() == "Typo"));
    mHPDLab->setVisible(!isTypo && visible);
    mHPDEdit->setVisible(!isTypo && visible);

    mDrawing->setVisible(visible);
    mResultsText->setVisible(visible);
    QWidget::setVisible(visible);
}

void CalibrationView::resizeEvent(QResizeEvent* e)
{
    Q_UNUSED(e);
    if (!mDate.mPlugin || width()<0 || height()<0) {
        setVisible(false);
        return;

    } else
        setVisible(true);


//QWidget::resizeEvent(e);
    update();
    //repaint();

}
void CalibrationView::paintEvent(QPaintEvent* e)
{
    Q_UNUSED(e);
    
    const int graphLeft (mButtonWidth);//mImageSaveBut->x() + mImageSaveBut->width());
    const int graphWidth (width() - graphLeft);

    QPainter p(this);
    //p.setRenderHint(QPainter::Antialiasing); // not necessary
    // drawing a background under button
    p.fillRect(QRect(0, 0, graphLeft, height()), QColor(50, 50, 50));

    // drawing a background under curve
    p.fillRect(QRect(graphLeft, 0, graphWidth, height()), Qt::white);

    updateLayout();

}

void CalibrationView::updateLayout()
{
    if (!mCalibGraph->hasCurve()) {
        mDrawing->setGeometry(mButtonWidth, 0, 0, 0);
        return;
    }
 /*   if (!mDate.mPlugin || width()<0 || height()<0) {
        setVisible(false);
        return;

    } else
        setVisible(true);
*/

    QFontMetrics fm (font());
    const int textHeight (fm.height() + 3);
    const int verticalSpacer (fm.height());

    //Position of Widget
    int y (0);

    mImageSaveBut->setGeometry(0, y, mButtonWidth, mButtonWidth);
    y += mImageSaveBut->height();
    mImageClipBut->setGeometry(0, y, mButtonWidth, mButtonWidth);
    y += mImageClipBut->height();
    mResultsClipBut->setGeometry(0, y, mButtonWidth, mButtonWidth);
    y += mResultsClipBut->height();


    const int separatorHeight (height()- y - 6*textHeight - 6* verticalSpacer);
    frameSeparator->setGeometry(0, y, mButtonWidth, separatorHeight);
    y += frameSeparator->height() + verticalSpacer;

    mStartLab->setGeometry(0, y, mButtonWidth, textHeight);
    y += mStartLab->height();
    mStartEdit->setGeometry(3, y, mButtonWidth-6, textHeight);
    y += mStartEdit->height() + verticalSpacer;
    mEndLab->setGeometry(0, y, mButtonWidth, textHeight);
    y += mEndLab->height();
    mEndEdit->setGeometry(3, y, mButtonWidth-6, textHeight);
    y += mEndEdit->height() + 3*verticalSpacer;
    mHPDLab->setGeometry(0, y, mButtonWidth, textHeight);
    y += mHPDLab->height();
    mHPDEdit->setGeometry(3, y, mButtonWidth-6, textHeight);

    const int graphLeft = mImageSaveBut->x() + mImageSaveBut->width();
    const int graphWidth = width() - graphLeft;

    const int resTextH = 5 * fm.height();
    mDrawing->setGeometry(graphLeft, 0, graphWidth, height() - resTextH);
    mResultsText->setGeometry(graphLeft + 20, mDrawing->y() + mDrawing->height(), graphWidth - 40 , resTextH);
    mResultsText->setAutoFillBackground(true);

}



