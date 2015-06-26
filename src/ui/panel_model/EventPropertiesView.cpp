#include "EventPropertiesView.h"
#include "ColorPicker.h"
#include "Event.h"
#include "EventKnown.h"
#include "DatesList.h"
#include "Button.h"
#include "Label.h"
#include "LineEdit.h"
#include "RadioButton.h"
#include "GraphView.h"
#include "Painting.h"
#include "MainWindow.h"
#include "Project.h"
#include "../PluginAbstract.h"
#include "PluginManager.h"
#include "StdUtilities.h"
#include "QtUtilities.h"
#include "ModelUtilities.h"
#include "PluginOptionsDialog.h"
#include <QtWidgets>


EventPropertiesView::EventPropertiesView(QWidget* parent, Qt::WindowFlags flags):QWidget(parent, flags),
mToolbarH(60)
{
    minimumHeight =0;
    
    

    // ------------- commun with defautlt Event and Bound ----------
    mNameLab = new Label(tr("Name") + " :", this);
    
    mNameEdit = new LineEdit(this);
    mNameEdit->setStyleSheet("QLineEdit { border-radius: 5px; }");
    mNameEdit->setAlignment(Qt::AlignHCenter);
    
    mColorLab = new Label(tr("Color") + " :", this);
    mColorPicker = new ColorPicker(Qt::black, this);
    //mColorPicker ->  QWidget::setStyleSheet("QLineEdit { border-radius: 5px; }");
    
    connect(mNameEdit, SIGNAL(textEdited(const QString&)), this, SLOT(updateEventName(const QString&)));
    connect(mColorPicker, SIGNAL(colorChanged(QColor)), this, SLOT(updateEventColor(QColor)));
    
    // Event default propreties Window mEventView
    mEventView = new QWidget(this);
    mMethodLab = new Label(tr("Method") + " :", mEventView);
    mMethodCombo = new QComboBox(mEventView);
    
    mMethodCombo->addItem(ModelUtilities::getEventMethodText(Event::eDoubleExp));
    mMethodCombo->addItem(ModelUtilities::getEventMethodText(Event::eBoxMuller));
    mMethodCombo->addItem(ModelUtilities::getEventMethodText(Event::eMHAdaptGauss));
    
   
    connect(mMethodCombo, SIGNAL(activated(int)), this, SLOT(updateEventMethod(int)));
    
    minimumHeight += mEventView->height();
    // -------------
    
    mDatesList = new DatesList(mEventView);
    connect(mDatesList, SIGNAL(calibRequested(const QJsonObject&)), this, SIGNAL(updateCalibRequested(const QJsonObject&)));
    
    // -------------

    const QList<PluginAbstract*>& plugins = PluginManager::getPlugins();
    //int totalH=0;
    for(int i=0; i<plugins.size(); ++i)
    {
        
        Button* button = new Button(plugins[i]->getName(), mEventView);
        button->setIcon(plugins[i]->getIcon());
        button->setFlatVertical();
        connect(button, SIGNAL(clicked()), this, SLOT(createDate()));
        
        //minimumHeight+=button->height();
        
        
        if(plugins[i]->doesCalibration())
            mPluginButs1.append(button);
        else
            mPluginButs2.append(button);
    }
    
    mDeleteBut = new Button(tr("Delete"), mEventView);
    mDeleteBut->setIcon(QIcon(":delete.png"));
    mDeleteBut->setFlatVertical();
    connect(mDeleteBut, SIGNAL(clicked()), this, SLOT(deleteSelectedDates()));
    minimumHeight+=mDeleteBut->height();
    
    mRecycleBut = new Button(tr("Restore"), mEventView);
    mRecycleBut->setIcon(QIcon(":restore.png"));
    mRecycleBut->setFlatVertical();
    connect(mRecycleBut, SIGNAL(clicked()), this, SLOT(recycleDates()));
    minimumHeight+=mRecycleBut->height();
    // ---------------
    
    mCalibBut = new Button(tr("Calibrate"), mEventView);
    mCalibBut->setIcon(QIcon(":results_w.png"));
    mCalibBut->setFlatVertical();
    mCalibBut->setCheckable(true);
    minimumHeight+=mCalibBut->height();
    
    mOptsBut = new Button(tr("Options"), mEventView);
    mOptsBut->setIcon(QIcon(":settings_w.png"));
    mOptsBut->setFlatVertical();
    minimumHeight+=mOptsBut->height();
    
    mMergeBut = new Button(tr("Combine"), mEventView);
    mMergeBut->setFlatVertical();
    mMergeBut->setEnabled(false);
    mMergeBut->setVisible(false);
    minimumHeight+=mMergeBut->height();
    
    mSplitBut = new Button(tr("Split"), mEventView);
    mSplitBut->setFlatVertical();
    mSplitBut->setEnabled(false);
    mSplitBut->setVisible(false);
    minimumHeight+=mSplitBut->height();
    
    connect(mCalibBut, SIGNAL(toggled(bool)), this, SIGNAL(showCalibRequested(bool)));
    connect(mOptsBut,  SIGNAL(clicked()), this, SLOT(showDatesOptions()));
    connect(mMergeBut, SIGNAL(clicked()), this, SLOT(sendMergeSelectedDates()));
    connect(mSplitBut, SIGNAL(clicked()), this, SLOT(sendSplitDate()));

    // --------------- Case of Event is a Bound -> Bound properties windows---------------------------
    
    mBoundView = new QWidget(this);
    
    mKnownFixedLab = new Label(tr("Value") + " :", mBoundView);
    mKnownStartLab = new Label(tr("Start") + " :", mBoundView);
    mKnownEndLab   = new Label(tr("End")   + " :", mBoundView);
    
    mKnownFixedRadio   = new RadioButton(tr("Fixed")   + " :", mBoundView);
    mKnownUniformRadio = new RadioButton(tr("Uniform") + " :", mBoundView);
    
    connect(mKnownFixedRadio, SIGNAL(clicked())  , this, SLOT(updateKnownType()));
    connect(mKnownUniformRadio, SIGNAL(clicked()), this, SLOT(updateKnownType()));
    
    mKnownFixedEdit = new LineEdit(mBoundView);
    mKnownStartEdit = new LineEdit(mBoundView);
    mKnownEndEdit   = new LineEdit(mBoundView);
    
    QDoubleValidator* doubleValidator = new QDoubleValidator();
    doubleValidator->setDecimals(2);
    //mKnownFixedEdit->setValidator(doubleValidator);
    
    mKnownGraph = new GraphView(mBoundView);
    
    mKnownGraph->setRendering(GraphView::eHD);
    mKnownGraph->showAxisArrows(true);
    mKnownGraph->showAxisLines(true);
    mKnownGraph->setXAxisMode(GraphView::eMinMax);
    mKnownGraph->setYAxisMode(GraphView::eMinMax);
    
    connect(mKnownFixedEdit, SIGNAL(textEdited(const QString&)), this, SLOT(updateKnownFixed(const QString&)));
    connect(mKnownStartEdit, SIGNAL(textEdited(const QString&)), this, SLOT(updateKnownUnifStart()));
    connect(mKnownEndEdit, SIGNAL(textEdited(const QString&)), this, SLOT(updateKnownUnifEnd()));
    
    setEvent(QJsonObject());
    
/*    qDebug()<<"minimumHeight"<<minimumHeight;
    qDebug()<<"EventPropertiesView::sortie constructeur width"<<width()<<" ; height"<<height();
*/    updateLayout();
}

EventPropertiesView::~EventPropertiesView()
{
    
}

#pragma mark Event Managment
void EventPropertiesView::setEvent(const QJsonObject& event)
{
    //qDebug() << "Event Property view updated";
    mEvent = event;
    updateEvent();
}

void EventPropertiesView::updateEvent()
{
    mDatesList->setEvent(mEvent);
    //qDebug()<<"EventPropertiesView::updateEvent()";
    bool empty = mEvent.isEmpty();
    
    mNameEdit   ->setEnabled(!empty);
    mColorPicker->setEnabled(!empty);
    mMethodCombo->setEnabled(!empty);
/*    mDeleteBut->setEnabled(!empty);
    mRecycleBut->setEnabled(!empty);
    mCalibBut->setEnabled(!empty);
*/    
    for(int i=0; i<mPluginButs1.size(); ++i)
        mPluginButs1[i]->setEnabled(!empty);
    
    for(int i=0; i<mPluginButs2.size(); ++i)
        mPluginButs2[i]->setEnabled(!empty);
    
    if(empty)
    {
        mEventView->setVisible(true);
        mBoundView->setVisible(false);
        mNameEdit->setText("");
        mMethodCombo->setCurrentIndex(0);
    }
    else
    {
        Event::Type type = (Event::Type)mEvent[STATE_EVENT_TYPE].toInt();
        QString name = mEvent[STATE_NAME].toString();
        QColor color(mEvent[STATE_COLOR_RED].toInt(),
                     mEvent[STATE_COLOR_GREEN].toInt(),
                     mEvent[STATE_COLOR_BLUE].toInt());
        
        if(name != mNameEdit->text())
            mNameEdit->setText(name);
        mColorPicker->setColor(color);
        
        mEventView->setVisible(type == Event::eDefault);
        mBoundView->setVisible(type == Event::eKnown);
        
        if(type == Event::eDefault)
        {
            mMethodCombo->setCurrentIndex(mEvent[STATE_EVENT_METHOD].toInt());
        }
        else if(type == Event::eKnown)
        {
            EventKnown::KnownType knownType = (EventKnown::KnownType)mEvent[STATE_EVENT_KNOWN_TYPE].toInt();
            
            mKnownFixedRadio   -> setChecked(knownType == EventKnown::eFixed);
            mKnownUniformRadio -> setChecked(knownType == EventKnown::eUniform);
            
            mKnownFixedEdit -> setText(QString::number(mEvent[STATE_EVENT_KNOWN_FIXED].toDouble()));
            mKnownStartEdit -> setText(QString::number(mEvent[STATE_EVENT_KNOWN_START].toDouble()));
            mKnownEndEdit   -> setText(QString::number(mEvent[STATE_EVENT_KNOWN_END].toDouble()));
            
            updateKnownControls();
            updateKnownGraph();
        }
    }
    updateLayout();
    update();
}

const QJsonObject& EventPropertiesView::getEvent() const
{
    return mEvent;
}

#pragma mark Event Properties
void EventPropertiesView::updateEventName(const QString& name)
{
    QJsonObject event = mEvent;
    event[STATE_NAME] = name;
    MainWindow::getInstance()->getProject()->updateEvent(event, tr("Event name updated"));
}

void EventPropertiesView::updateEventColor(QColor color)
{
    QJsonObject event = mEvent;
    event[STATE_COLOR_RED] = color.red();
    event[STATE_COLOR_GREEN] = color.green();
    event[STATE_COLOR_BLUE] = color.blue();
    MainWindow::getInstance()->getProject()->updateEvent(event, tr("Event color updated"));
}

void EventPropertiesView::updateEventMethod(int index)
{
    QJsonObject event = mEvent;
    event[STATE_EVENT_METHOD] = index;
    MainWindow::getInstance()->getProject()->updateEvent(event, tr("Event method updated"));
}

#pragma mark Event Known Properties
void EventPropertiesView::updateKnownType()
{
    if(mEvent[STATE_EVENT_TYPE].toInt() == Event::eKnown)
    {
        EventKnown::KnownType type = EventKnown::eFixed;
        if(mKnownUniformRadio->isChecked())
            type = EventKnown::eUniform;
        
        if(mEvent[STATE_EVENT_KNOWN_TYPE].toInt() != type)
        {
            QJsonObject event = mEvent;
            event[STATE_EVENT_KNOWN_TYPE] = type;
            MainWindow::getInstance()->getProject()->updateEvent(event, tr("Bound type updated"));
        }
    }
}

void EventPropertiesView::updateKnownFixed(const QString& text)
{
    QJsonObject event = mEvent;
    event[STATE_EVENT_KNOWN_FIXED] = round(text.toDouble());
    MainWindow::getInstance()->getProject()->updateEvent(event, tr("Bound fixed value updated"));
}
void EventPropertiesView::updateKnownUnifStart()
{
    QJsonObject event = mEvent;
    event[STATE_EVENT_KNOWN_START] = round(mKnownStartEdit->text().toDouble());
    MainWindow::getInstance()->getProject()->updateEvent(event, tr("Bound min updated"));
}

void EventPropertiesView::updateKnownUnifEnd()
{
    QJsonObject event = mEvent;
    event[STATE_EVENT_KNOWN_END] = round(mKnownEndEdit->text().toDouble());
    MainWindow::getInstance()->getProject()->updateEvent(event, tr("Bound max updated"));
}

void EventPropertiesView::loadKnownCsv()
{
    /*if(mEvent)
     {
     QString currentDir = MainWindow::getInstance()->getCurrentPath();
     QString path = QFileDialog::getOpenFileName(qApp->activeWindow(), tr("Open CSV File"), currentDir, tr("CSV File (*.csv)"));
     
     if(!path.isEmpty())
     {
     QFileInfo info(path);
     QString dirPath = info.absolutePath();
     MainWindow::getInstance()->setCurrentPath(dirPath);
     
     QList<QStringList> csv = readCSV(path, ";");
     qDebug() << csv;
     qDebug() << path;
     qDebug() << csv.size();
     
     EventKnown* e = dynamic_cast<EventKnown*>(mEvent);
     if(e)
     {
     for(int i=0; i<csv.size(); ++i)
     {
     double x = csv[i][0].replace(",", ".").toDouble();
     double y = csv[i][1].replace(",", ".").toDouble();
     e->mValues[x] = y;
     }
     e->mValues = normalize_map(e->mValues);
     
     Project* project = MainWindow::getInstance()->getProject();
     e->updateValues(project->mSettings.mTmin, project->mSettings.mTmax, project->mSettings.mStep);
     updateKnownGraph();
     }
     }
     }*/
}

void EventPropertiesView::updateKnownGraph()
{
    mKnownGraph->removeAllCurves();
    
    if(mEvent[STATE_EVENT_TYPE].toInt() == Event::eKnown)
    {
        Project* project = MainWindow::getInstance()->getProject();
        QJsonObject state = project->state();
        QJsonObject settings = state[STATE_SETTINGS].toObject();
        double tmin = settings[STATE_SETTINGS_TMIN].toDouble();
        double tmax = settings[STATE_SETTINGS_TMAX].toDouble();
        double step = settings[STATE_SETTINGS_STEP].toDouble();
        EventKnown event = EventKnown::fromJson(mEvent);
        event.updateValues(tmin, tmax,step );
        
        mKnownGraph->setRangeX(tmin,tmax);
        
        //qDebug() << "EventPropertiesView::updateKnownGraph()"<<event.mValues.size();
        mKnownGraph->setCurrentX(tmin,tmax);
        
        double max = map_max_value(event.mValues);
        max = (max == 0) ? 1 : max;
        mKnownGraph->setRangeY(0, max);
        mKnownGraph->showAxisArrows(false);
        mKnownGraph->showAxisLines(false);
        mKnownGraph->setXAxisMode(GraphView::eHidden);
        //mKnownGraph->setYAxisMode(GraphView::eHidden);
        
        // draw the calibrate curve on the rigth hand panel
        GraphCurve curve;
        curve.mName = "Known";
        curve.mData = event.mValues;
        
        curve.mPen.setColor(Painting::mainColorLight);
        curve.mFillUnder = true;
        if(event.knownType() == EventKnown::eUniform)
        {
            curve.mIsHisto = true;
            curve.mIsRectFromZero = true;
        }
        else if(event.knownType() == EventKnown::eFixed)
        {
            curve.mIsHisto = false;
            curve.mIsRectFromZero = true;
        }
        mKnownGraph->addCurve(curve);
    }
}

void EventPropertiesView::updateKnownControls()
{
    if(mKnownFixedRadio->isChecked())
    {
        mKnownFixedEdit->setEnabled(true);
        mKnownStartEdit->setEnabled(false);
        mKnownEndEdit->setEnabled(false);
    }
    else if(mKnownUniformRadio->isChecked())
    {
        mKnownFixedEdit->setEnabled(false);
        mKnownStartEdit->setEnabled(true);
        mKnownEndEdit->setEnabled(true);
    }
}

void EventPropertiesView::hideCalibration()
{
//    mCalibBut->setChecked(false);
}

#pragma mark Event Data
void EventPropertiesView::createDate()
{
    if(!mEvent.isEmpty())
    {
        Button* but = dynamic_cast<Button*>(sender());
        if(but)
        {
            Project* project = MainWindow::getInstance()->getProject();
            const QList<PluginAbstract*>& plugins = PluginManager::getPlugins();
            
            for(int i=0; i<plugins.size(); ++i)
            {
                if(plugins[i]->getName() == but->text())
                {
                    Date date = project->createDateFromPlugin(plugins[i]);
                    if(!date.isNull())
                        project->addDate(mEvent[STATE_ID].toInt(), date.toJson());
                }
            }
        }
    }
}

void EventPropertiesView::deleteSelectedDates()
{
    QList<QListWidgetItem*> items = mDatesList->selectedItems();
    QList<int> indexes;
    for(int i=0; i<items.size(); ++i)
        indexes.push_back(mDatesList->row(items[i]));
    
    MainWindow::getInstance()->getProject()->deleteDates(mEvent[STATE_ID].toInt(), indexes);
}

void EventPropertiesView::recycleDates()
{
    MainWindow::getInstance()->getProject()->recycleDates(mEvent[STATE_ID].toInt());
}

#pragma mark Merge / Split
void EventPropertiesView::updateDatesSelection()
{
    /*QList<Date*> selectedDates;
    QString pluginName;
    bool mergeable = true;
    bool splittable = false;
    for(int i=0; i<mEvent->mDates.size(); ++i)
    {
        if(mEvent->mDates[i]->mIsSelected)
        {
            selectedDates.append(mEvent->mDates[i]);
            
            QString plgName = mEvent->mDates[i]->mPlugin->getName();
            if(pluginName.isEmpty())
                pluginName = plgName;
            if(plgName != pluginName)
                mergeable = false;
            
            if(mEvent->mDates[i]->mSubDates.size() > 0)
            {
                mergeable = false;
                splittable = true;
            }
        }
    }
    mDeleteBut->setEnabled(selectedDates.size() > 0);
    mMergeBut->setEnabled(selectedDates.size() >= 2 && mergeable);
    mSplitBut->setEnabled(selectedDates.size() == 1 && splittable);*/
}

void EventPropertiesView::sendMergeSelectedDates()
{
    //emit mergeSelectedDates(mEvent);
}

void EventPropertiesView::sendSplitDate()
{
    /*for(int i=0; i<mEvent->mDates.size(); ++i)
    {
        if(mEvent->mDates[i]->mIsSelected)
        {
            emit splitDate(mEvent->mDates[i]);
            return;
        }
    }*/
}


#pragma mark Dates Options
void EventPropertiesView::showDatesOptions()
{
    // ---------------------------------------------------------
    //  TODO : each plugin should return (or not) a view with
    //  a set of options that can be applied to all dates of this type.
    //  For now, we just use a simple temporary dialog
    // ---------------------------------------------------------
    
    PluginOptionsDialog dialog(qApp->activeWindow(), Qt::Sheet);
    
    /*const QList<PluginAbstract*>& plugins = PluginManager::getPlugins();
    for(int i=0; i<plugins.size(); ++i)
    {
        QWidget* plgOptsView = plugins[i]->optionsView();
        if(plgOptsView)
        {
            dialog.addOptions(plugins[i]->getName(), plgOptsView);
        }
    }*/
    
    if(dialog.exec() == QDialog::Accepted)
    {
        QString ref14C = dialog.getC14Ref();
        Project* project = MainWindow::getInstance()->getProject();
        project->updateAll14CData(ref14C);
    }
}


#pragma mark Layout
void EventPropertiesView::paintEvent(QPaintEvent* e)
{
    Q_UNUSED(e);
    QPainter p(this);
    p.fillRect(rect(), QColor(200, 200, 200));
}

void EventPropertiesView::resizeEvent(QResizeEvent* e)
{
    Q_UNUSED(e);
    updateLayout();
}

void EventPropertiesView::updateLayout()
{
  
    this->setGeometry(0, mToolbarH, this->parentWidget()->width(),this->parentWidget()->height()-mToolbarH);
    
    int mTalonLabel = 7;
    int mTalonBox = 80;
    
    int mLabelWidth = 50;
    
    int butPluginWidth = 80;
    int butPluginHeigth = 50;
    
    int mBoxWidth = this->width() - butPluginWidth-int(floor(2*mTalonLabel));
    
    int mBoxHeigth = int(floor( (mToolbarH-3*mTalonLabel) /2 ));//20;
    

    
    if(width() < 100)
    {
        butPluginWidth = 0;
    }
    
    
    
    // place the QLabel
    // mNameLab, mColorLab, mMethodLab, belong to mEventView
    int y = mTalonLabel;
    mNameLab  -> setGeometry(mTalonLabel, y, mLabelWidth, mBoxHeigth);
    mNameEdit -> setGeometry(mTalonBox, y, mBoxWidth, mBoxHeigth);
    y += mNameEdit -> height() + mTalonLabel;
    
    mColorLab    -> setGeometry(mTalonLabel, y, mLabelWidth, mBoxHeigth);
    mColorPicker -> setGeometry(mTalonBox, 2*mTalonLabel + mBoxHeigth, mBoxWidth, mBoxHeigth);
    //mColorPicker->etGeometry(mTalonBox, 2*mTalonLabel + mBoxHeigth, mBoxWidth, mBoxHeigth);
    y += mColorPicker -> height() + mTalonLabel;
    
    mEventView->setGeometry(0, y, this->width(), this->height()-y);
    mBoundView->setGeometry(0, y, this->width(), this->height()-y);
    
    // Event properties view :
    y = 0;
    
    mMethodLab   -> setGeometry(mTalonLabel, y  , mLabelWidth, mBoxHeigth);
    mMethodCombo -> setGeometry(mTalonBox-4, y-4, mBoxWidth+8, mBoxHeigth+8);
    y += mMethodCombo->height() + mTalonLabel;
    
    QRect listRect(0, y, mEventView->width() - butPluginWidth, mEventView->height() - y);
    
    mDatesList->setGeometry(listRect);
    
    int x = listRect.width();
    //int y = listRect.y();
    
    for(int i=0; i<mPluginButs1.size(); ++i)
    {
        mPluginButs1[i]->setGeometry(x, y, butPluginWidth, butPluginHeigth);
        y += butPluginHeigth;
    }
    
    
    for(int i=0; i<mPluginButs2.size(); ++i)
    {
        mPluginButs2[i]->setGeometry(x, y, butPluginWidth, butPluginHeigth);
        y += butPluginHeigth;
    }
    
    y +=mTalonBox;
    mCalibBut  ->setGeometry(x, mEventView->height() -4*butPluginHeigth, butPluginWidth,butPluginHeigth);
    y += butPluginHeigth;
    mOptsBut   ->setGeometry(x, mEventView->height() -3*butPluginHeigth, butPluginWidth,butPluginHeigth);
    y += butPluginHeigth;
    mDeleteBut ->setGeometry(x, mEventView->height() -2*butPluginHeigth, butPluginWidth,butPluginHeigth);
    y += butPluginHeigth;
    mRecycleBut->setGeometry(x, mEventView->height() - 0-butPluginHeigth  , butPluginWidth, butPluginHeigth);
 
    // Known view : Used with Bound
    
    y = 0;//mMethodCombo->y() + mTalonLabel;;
    QRect r = this->rect();
    int m = mTalonLabel;//5;
    int w1 = 80;
    int lineH = 20;
    int w2 = r.width() - w1 - 3*m;
    mKnownFixedRadio->setGeometry(m, y, r.width() - 2*m, lineH);
    mKnownFixedLab  ->setGeometry(m, y += (lineH + m), w1, lineH);
    mKnownFixedEdit ->setGeometry(w1 + 2*m, y, w2, lineH);
    
    mKnownUniformRadio->setGeometry(m, y += (lineH + m), r.width() - 2*m, lineH);
    mKnownStartLab->setGeometry(m, y += (lineH + m), w1, lineH);
    mKnownStartEdit->setGeometry(w1 + 2*m, y, w2, lineH);
    mKnownEndLab->setGeometry(m, y += (lineH + m), w1, lineH);
    mKnownEndEdit->setGeometry(w1 + 2*m, y, w2, lineH);
    
    mKnownGraph->setGeometry(m, y += (lineH + m), r.width() - 2*m, 100);
/* qDebug()<<"EventPropertiesView::sortie update width"<<width()<<" ; height"<<height();
     qDebug()<<"EventPropertiesView::sortie constructeur mEventView width"<<mEventView-> width()<<" ; height"<<mEventView-> height();
   // update(); */
}

