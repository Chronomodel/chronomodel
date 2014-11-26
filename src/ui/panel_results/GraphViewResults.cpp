#include "GraphViewResults.h"
#include "GraphView.h"
#include "Button.h"
#include "Painting.h"
#include "QtUtilities.h"
#include "MainWindow.h"
#include <QtWidgets>
#include <QtSvg>



#pragma mark Constructor / Destructor

GraphViewResults::GraphViewResults(QWidget *parent):QWidget(parent),
mCurrentResult(eHisto),
mCurrentVariable(eTheta),
mShowAllChains(true),
mShowHPD(false),
mThresholdHPD(95),
mShowCalib(false),
mShowWiggle(false),
mMainColor(QColor(50, 50, 50)),
mMargin(5),
mLineH(20),
mGraphLeft(130)
{
    setMouseTracking(true);
    
    mGraph = new GraphView(this);
    
    mGraph->showAxis(false);
    mGraph->showScrollBar(false);
    mGraph->showYValues(true);
    mGraph->setMarginRight(10);
    mGraph->setRangeY(0, 1);
    
    mTextArea = new QTextEdit(this);
    mTextArea->setFrameStyle(QFrame::NoFrame);
    QPalette palette = mTextArea->palette();
    palette.setColor(QPalette::Base, QColor(0, 0, 0, 200));
    palette.setColor(QPalette::Text, Qt::white);
    mTextArea->setPalette(palette);
    QFont font = mTextArea->font();
    font.setPointSizeF(pointSize(11));
    mTextArea->setFont(font);
    mTextArea->setText(tr("Nothing to display"));
    mTextArea->setVisible(false);
    
    mImageSaveBut = new Button(tr("Save"), this);
    mImageSaveBut->setIcon(QIcon(":picture_save.png"));
    mImageSaveBut->setFlatVertical();
    mImageSaveBut->setToolTip(tr("Save image as file"));
    
    mImageClipBut = new Button(tr("Copy"), this);
    mImageClipBut->setIcon(QIcon(":picture_copy.png"));
    mImageClipBut->setFlatVertical();
    mImageClipBut->setToolTip(tr("Copy image to clipboard"));
    
    mResultsClipBut = new Button(tr("Copy"), this);
    mResultsClipBut->setIcon(QIcon(":text.png"));
    mResultsClipBut->setFlatVertical();
    mImageClipBut->setToolTip(tr("Copy text results to clipboard"));
    
    mDataSaveBut = new Button(tr("Save"), this);
    mDataSaveBut->setIcon(QIcon(":data.png"));
    mDataSaveBut->setFlatVertical();
    mDataSaveBut->setToolTip(tr("Save graph data to file"));
    
    mAnimation = new QPropertyAnimation();
    mAnimation->setPropertyName("geometry");
    mAnimation->setDuration(200);
    mAnimation->setTargetObject(this);
    mAnimation->setEasingCurve(QEasingCurve::Linear);
    
    connect(mImageSaveBut, SIGNAL(clicked()), this, SLOT(saveAsImage()));
    connect(mImageClipBut, SIGNAL(clicked()), this, SLOT(imageToClipboard()));
    connect(mResultsClipBut, SIGNAL(clicked()), this, SLOT(resultsToClipboard()));
    connect(mDataSaveBut, SIGNAL(clicked()), this, SLOT(saveGraphData()));
    
    setSizePolicy(QSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred));
}

GraphViewResults::~GraphViewResults()
{
    
}

void GraphViewResults::setResultToShow(Result result, Variable variable, bool showAllChains, const QList<bool>& showChainList, bool showHpd, int threshold, bool showCalib, bool showWiggle)
{
    mCurrentResult = result;
    mCurrentVariable = variable;
    mShowAllChains = showAllChains;
    mShowChainList = showChainList;
    mShowHPD = showHpd;
    mThresholdHPD = threshold;
    mShowCalib = showCalib;
    mShowWiggle = showWiggle;
    refresh();
}

void GraphViewResults::toggle(const QRect& targetGeometry)
{
    if(geometry() != targetGeometry)
    {
        //qDebug() << "Graph From : " << geometry() << ", To : " << targetGeometry;
        
        mAnimation->setStartValue(geometry());
        mAnimation->setEndValue(targetGeometry);
        mAnimation->start();
    }
}

void GraphViewResults::setSettings(const ProjectSettings& settings)
{
    mSettings = settings;
}

void GraphViewResults::setMCMCSettings(const MCMCSettings& mcmc, const QList<Chain>& chains)
{
    mMCMCSettings = mcmc;
    mChains = chains;
}

void GraphViewResults::setRange(float min, float max)
{
    mGraph->setRangeX(min, max);
}

void GraphViewResults::zoom(float min, float max)
{
    mGraph->zoomX(min, max);
}

void GraphViewResults::setMainColor(const QColor& color)
{
    mMainColor = color;
    update();
}

void GraphViewResults::saveAsImage()
{
    QString filter = tr("Image (*.png);;Scalable Vector Graphics (*.svg)");
    QString fileName = QFileDialog::getSaveFileName(qApp->activeWindow(),
                                                    tr("Save graph image as..."),
                                                    MainWindow::getInstance()->getCurrentPath(),
                                                    filter);
    if(!fileName.isEmpty())
    {
        bool asSvg = fileName.endsWith(".svg");
        if(asSvg)
        {
            QSvgGenerator svgGen;
            svgGen.setFileName(fileName);
            svgGen.setSize(mGraph->size());
            svgGen.setViewBox(QRect(0, 0, mGraph->width(), mGraph->height()));
            QPainter p(&svgGen);
            p.setRenderHint(QPainter::Antialiasing);
            mGraph->render(&p);
        }
        else
        {
            QImage image(mGraph->size(), QImage::Format_ARGB32);
            image.fill(Qt::transparent);
            QPainter p(&image);
            p.setRenderHint(QPainter::Antialiasing);
            mGraph->render(&p);
            image.save(fileName, "PNG");
        }
        QFileInfo fileInfo(fileName);
        MainWindow::getInstance()->setCurrentPath(fileInfo.dir().absolutePath());
    }
}

void GraphViewResults::imageToClipboard()
{
    QClipboard* clipboard = QApplication::clipboard();
    clipboard->setPixmap(mGraph->grab());
}

void GraphViewResults::resultsToClipboard()
{
    QClipboard* clipboard = QApplication::clipboard();
    clipboard->setText(mResults);
}

void GraphViewResults::setNumericalResults(const QString& results)
{
    mResults = results;
    mTextArea->setText(mResults);
}

void GraphViewResults::showNumericalResults(bool show)
{
    if(mGraph->numCurves() != 0)
    {
        mTextArea->setVisible(show);
        if(show)
            mTextArea->raise();
    }
    else
        mTextArea->setVisible(false);
}

void GraphViewResults::paintEvent(QPaintEvent* e)
{
    Q_UNUSED(e);
    QPainter p(this);
    
    p.fillRect(0, 0, mGraphLeft, height(), mMainColor);
    
    p.setPen(QColor(200, 200, 200));
    p.drawLine(0, height(), width(), height());
}

void GraphViewResults::resizeEvent(QResizeEvent* e)
{
    Q_UNUSED(e);
    
    int h = height();
    int butMinH = 30;
    int butInlineMaxH = 50;
    //int bh = (height() - mLineH) / 2;
    //bh = qMin(bh, 100);
    
    bool showButs = (h >= mLineH + butMinH);
    
    mImageSaveBut->setVisible(showButs);
    mDataSaveBut->setVisible(showButs);
    mImageClipBut->setVisible(showButs);
    mResultsClipBut->setVisible(showButs);
    
    if(showButs)
    {
        int bw = mGraphLeft / 4;
        int bh = height() - mLineH;
        bh = qMin(bh, butInlineMaxH);
        
        mImageSaveBut->setGeometry(0, mLineH, bw, bh);
        mDataSaveBut->setGeometry(bw, mLineH, bw, bh);
        mImageClipBut->setGeometry(2*bw, mLineH, bw, bh);
        mResultsClipBut->setGeometry(3*bw, mLineH, bw, bh);
    }
    
    QRect graphRect(mGraphLeft, 0, width() - mGraphLeft, height()-1);
    if(h <= mLineH + butMinH)
    {
        mGraph->showYValues(false, true);
    }
    else
    {
        mGraph->showYValues(true);
    }
    mGraph->setGeometry(graphRect);
    mTextArea->setGeometry(graphRect);
}
