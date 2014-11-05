#include "GraphViewResults.h"
#include "GraphView.h"
#include "Button.h"
#include "ProjectManager.h"
#include "Project.h"
#include "Painting.h"
#include "QtUtilities.h"
#include <QtWidgets>



#pragma mark Constructor / Destructor

GraphViewResults::GraphViewResults(QWidget *parent):QWidget(parent),
mParentGraph(0),
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
    mGraph->setRangeY(0, 1);
    
    mImageBut = new Button(tr("Image"), this);
    mDataBut = new Button(tr("Export"), this);
    
    mAnimation = new QPropertyAnimation();
    mAnimation->setPropertyName("geometry");
    mAnimation->setDuration(200);
    mAnimation->setTargetObject(this);
    mAnimation->setEasingCurve(QEasingCurve::Linear);
    
    connect(mImageBut, SIGNAL(clicked()), this, SLOT(exportAsImage()));
    connect(mDataBut, SIGNAL(clicked()), this, SLOT(exportData()));
    
    setSizePolicy(QSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred));
}

GraphViewResults::~GraphViewResults()
{
    
}

void GraphViewResults::toggle(const QRect& targetGeometry)
{
    if(geometry() != targetGeometry)
    {
        qDebug() << "Graph From : " << geometry() << ", To : " << targetGeometry;
        
        mAnimation->setStartValue(geometry());
        mAnimation->setEndValue(targetGeometry);
        mAnimation->start();
    }
}

void GraphViewResults::setSettings(const ProjectSettings& settings)
{
    mSettings = settings;
}

void GraphViewResults::setMCMCSettings(const MCMCSettings& mcmc)
{
    mMCMCSettings = mcmc;
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

void GraphViewResults::exportAsImage()
{
    
}

void GraphViewResults::exportData()
{
    /*QTextEdit* edit = new QTextEdit();
     const QMap<double, double>& data = mGraph->curveData("graph");
     QMap<double, double>::const_iterator it;
     for(it = data.begin(); it != data.end(); ++it)
     {
     edit->append(QString::number(it->first) + "|" + QString::number(it->second) + "\n");
     }
     edit->show();*/
}

void GraphViewResults::paintEvent(QPaintEvent* e)
{
    Q_UNUSED(e);
    QPainter p(this);
    
    QLinearGradient grad(0, 0, 0, height());
    grad.setColorAt(0, mMainColor.lighter());
    grad.setColorAt(1, mMainColor);
    //p.fillRect(rect(), grad);
    
    p.fillRect(rect(), mMainColor);
    QColor foreCol = getContrastedColor(mMainColor);
    p.setPen(foreCol);
    
    p.setPen(Qt::black);
    p.drawRect(0, 0, mGraphLeft, height());
}

void GraphViewResults::resizeEvent(QResizeEvent* e)
{
    Q_UNUSED(e);
    
    mImageBut->setGeometry(mMargin, mMargin + mLineH, (mGraphLeft - 2*mMargin), mLineH);
    mDataBut->setGeometry(mMargin, 2*mMargin + 2*mLineH, (mGraphLeft - 2*mMargin), mLineH);
    
    mGraph->setGeometry(mGraphLeft, 0, width() - mGraphLeft, height());
}

