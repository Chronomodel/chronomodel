#include "EventPropertiesView.h"
#include "ColorPicker.h"
#include "Event.h"
#include "EventKnown.h"
#include "DatesList.h"
#include "Button.h"
//#include "RadioButton.h"
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
mToolbarH(100)
{
    minimumHeight =0;

    // ------------- commun with defautlt Event and Bound ----------
    mNameLab = new QLabel(tr("Name") + " :");
    
    mNameEdit = new QLineEdit();
    mNameEdit->setStyleSheet("QLineEdit { border-radius: 5px; }");
    mNameEdit->setAlignment(Qt::AlignHCenter);
    
    mColorLab = new QLabel(tr("Color") + " :");
    mColorPicker = new ColorPicker(Qt::black);
    
    mMethodLab = new QLabel(tr("Method") + " :");
    mMethodCombo = new QComboBox();
    
    mMethodCombo->addItem(ModelUtilities::getEventMethodText(Event::eDoubleExp));
    mMethodCombo->addItem(ModelUtilities::getEventMethodText(Event::eBoxMuller));
    mMethodCombo->addItem(ModelUtilities::getEventMethodText(Event::eMHAdaptGauss));
    
    connect(mNameEdit, SIGNAL(editingFinished()), this, SLOT(updateEventName()));
    connect(mColorPicker, SIGNAL(colorChanged(QColor)), this, SLOT(updateEventColor(QColor)));
    connect(mMethodCombo, SIGNAL(activated(int)), this, SLOT(updateEventMethod(int)));
    
    QGridLayout* grid = new QGridLayout();
    grid->setSpacing(6);
    grid->setContentsMargins(0, 0, 0, 0);
    grid->addWidget(mNameLab, 0, 0);
    grid->addWidget(mNameEdit, 0, 1);
    grid->addWidget(mColorLab, 1, 0);
    grid->addWidget(mColorPicker, 1, 1);
    grid->addWidget(mMethodLab, 2, 0);
    grid->addWidget(mMethodCombo, 2, 1);
    
    QVBoxLayout* topLayout = new QVBoxLayout();
    topLayout->setContentsMargins(10, 6, 10, 6);
    topLayout->addLayout(grid);
    topLayout->addStretch();
    
    mTopView = new QWidget(this);
    mTopView->setLayout(topLayout);
    mTopView->setFixedHeight(mToolbarH);
    
    
    // Event default propreties Window mEventView
    mEventView = new QWidget(this);
    
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
    
    mMergeBut = new Button(tr("Combine"), mEventView);
    mMergeBut->setFlatVertical();
    mMergeBut->setEnabled(false);
    //mMergeBut->setVisible(false);
    minimumHeight+=mMergeBut->height();
    
    mSplitBut = new Button(tr("Split"), mEventView);
    mSplitBut->setFlatVertical();
    mSplitBut->setEnabled(false);
    //mSplitBut->setVisible(false);
    minimumHeight+=mSplitBut->height();
    
    connect(mCalibBut, SIGNAL(toggled(bool)), this, SIGNAL(showCalibRequested(bool)));
    connect(mMergeBut, SIGNAL(clicked()), this, SLOT(sendMergeSelectedDates()));
    connect(mSplitBut, SIGNAL(clicked()), this, SLOT(sendSplitDate()));

    // --------------- Case of Event is a Bound -> Bound properties windows---------------------------
    
    mBoundView = new QWidget(this);
    
    mKnownFixedRadio   = new QRadioButton(tr("Fixed")   + " :", mBoundView);
    mKnownUniformRadio = new QRadioButton(tr("Uniform") + " :", mBoundView);
    
    connect(mKnownFixedRadio, SIGNAL(clicked())  , this, SLOT(updateKnownType()));
    connect(mKnownUniformRadio, SIGNAL(clicked()), this, SLOT(updateKnownType()));
    
    mKnownFixedEdit = new QLineEdit(mBoundView);
    mKnownStartEdit = new QLineEdit(mBoundView);
    mKnownEndEdit   = new QLineEdit(mBoundView);
    
    QDoubleValidator* doubleValidator = new QDoubleValidator();
    doubleValidator->setDecimals(2);
    //mKnownFixedEdit->setValidator(doubleValidator);
    
    mKnownGraph = new GraphView(mBoundView);
    mKnownGraph->setMinimumHeight(250);
    
    mKnownGraph->setRendering(GraphView::eHD);
    
    mKnownGraph->showXAxisArrow(true);
    mKnownGraph->showXAxisTicks(true);
    mKnownGraph->showXAxisSubTicks(true);
    mKnownGraph->showXAxisValues(true);
    
    mKnownGraph->showYAxisArrow(true);
    mKnownGraph->showYAxisTicks(false);
    mKnownGraph->showYAxisSubTicks(false);
    mKnownGraph->showYAxisValues(false);
    
    mKnownGraph->setXAxisMode(GraphView::eMinMax);
    mKnownGraph->setYAxisMode(GraphView::eMinMax);
    
    connect(mDatesList, SIGNAL(itemSelectionChanged()), this, SLOT(updateCombineAvailability()));
    connect(mKnownFixedEdit, SIGNAL(textEdited(const QString&)), this, SLOT(updateKnownFixed(const QString&)));
    connect(mKnownStartEdit, SIGNAL(textEdited(const QString&)), this, SLOT(updateKnownUnifStart()));
    connect(mKnownEndEdit, SIGNAL(textEdited(const QString&)), this, SLOT(updateKnownUnifEnd()));
    
    QFormLayout* fixedLayout = new QFormLayout();
    fixedLayout->addRow(tr("Value :"), mKnownFixedEdit);
    mFixedGroup = new QGroupBox();
    mFixedGroup->setLayout(fixedLayout);
    
    QFormLayout* uniformLayout = new QFormLayout();
    uniformLayout->addRow(tr("Start :"), mKnownStartEdit);
    uniformLayout->addRow(tr("End :"), mKnownEndEdit);
    mUniformGroup = new QGroupBox();
    mUniformGroup->setLayout(uniformLayout);
    
    
    QVBoxLayout* boundLayout = new QVBoxLayout();
    boundLayout->setContentsMargins(10, 6, 15, 6);
    boundLayout->setSpacing(10);
    boundLayout->addWidget(mKnownFixedRadio);
    boundLayout->addWidget(mFixedGroup);
    boundLayout->addWidget(mKnownUniformRadio);
    boundLayout->addWidget(mUniformGroup);
    boundLayout->addWidget(mKnownGraph);
    boundLayout->addStretch();
    mBoundView->setLayout(boundLayout);
    
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
        
        mMethodLab->setVisible(type == Event::eDefault);
        mMethodCombo->setVisible(type == Event::eDefault);
        
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
void EventPropertiesView::updateEventName()
{
    QJsonObject event = mEvent;
    event[STATE_NAME] = mNameEdit->text();
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
        
        // draw the calibrate curve on the rigth hand panel
       /* GraphCurve curve;
        curve.mName = "Known";
        curve.mData = event.mValues;
        
        curve.mPen = Painting::mainColorLight;
        curve.mBrush = Painting::mainColorLight;
        
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
        mKnownGraph->addCurve(curve);*/
        //---------------------

        GraphCurve curve;
        curve.mName = "Bound";
        curve.mBrush = Painting::mainColorLight;

        curve.mPen = QPen(Painting::mainColorLight, 2.f);
        
        curve.mIsHorizontalSections = true;
        qreal tLower;
        qreal tUpper;
        if(event.knownType() == EventKnown::eFixed) {
            tLower = event.fixedValue();
            tUpper = tLower;

        }
        else {
            tLower = event.uniformStart();
            tUpper = event.uniformEnd();
        }

        curve.mSections.append(qMakePair(tLower,tUpper));
        mKnownGraph->addCurve(curve);
        //---------------------
    }
}

void EventPropertiesView::updateKnownControls()
{
    mFixedGroup->setVisible(mKnownFixedRadio->isChecked());
    mUniformGroup->setVisible(mKnownUniformRadio->isChecked());
    
    /*if(mKnownFixedRadio->isChecked())
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
    }*/
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
void EventPropertiesView::updateCombineAvailability()
{
    bool mergeable = false;
    bool splittable = false;
    
    QJsonArray dates = mEvent[STATE_EVENT_DATES].toArray();
    QList<QListWidgetItem*> items = mDatesList->selectedItems();
    
    if(items.size() == 1){
        // Split?
        int idx = mDatesList->row(items[0]);
        QJsonObject date = dates[idx].toObject();
        if(date[STATE_DATE_SUB_DATES].toArray().size() > 0){
            splittable = true;
        }
    }else if(items.size() > 1 && dates.size() > 1){
        // Combine?
        mergeable = true;
        PluginAbstract* plugin = 0;
        
        for(int i=0; i<items.size(); ++i){
            int idx = mDatesList->row(items[i]);
            if(idx < dates.size()){
                QJsonObject date = dates[idx].toObject();
                // If selected date already has subdates, it cannot be combined :
                if(date[STATE_DATE_SUB_DATES].toArray().size() > 0){
                    mergeable = false;
                    break;
                }
                // If selected dates have different plugins, they cannot be combined :
                PluginAbstract* plg = PluginManager::getPluginFromId(date[STATE_DATE_PLUGIN_ID].toString());
                if(plugin == 0)
                    plugin = plg;
                else if(plg != plugin){
                    mergeable = false;
                    break;
                }
            }
        }
        // Dates are not combined yet and are from the same plugin.
        // We should now ask the plugin if they are combinable (they use the same ref curve for example...)
        if(mergeable && plugin != 0){
            // This could be used instead to disable the "combine" button if dates cannot be combined.
            // We prefer letting the user combine them and get an error message explaining why they cannot be combined!
            // Check Plugin14C::mergeDates as example
            //mergeable = plugin->areDatesMergeable(dates);
            
            mergeable = true;
        }
    }
    mMergeBut->setEnabled(mergeable);
    mSplitBut->setEnabled(splittable);
}

void EventPropertiesView::sendMergeSelectedDates()
{
    QJsonArray dates = mEvent[STATE_EVENT_DATES].toArray();
    QList<QListWidgetItem*> items = mDatesList->selectedItems();
    QList<int> dateIds;
    
    for(int i=0; i<items.size(); ++i){
        int idx = mDatesList->row(items[i]);
        if(idx < dates.size()){
            QJsonObject date = dates[idx].toObject();
            dateIds.push_back(date[STATE_ID].toInt());
        }
    }
    emit mergeDatesRequested(mEvent[STATE_ID].toInt(), dateIds);
}

void EventPropertiesView::sendSplitDate()
{
    QJsonArray dates = mEvent[STATE_EVENT_DATES].toArray();
    QList<QListWidgetItem*> items = mDatesList->selectedItems();
    if(items.size() > 0){
        int idx = mDatesList->row(items[0]);
        if(idx < dates.size()){
            QJsonObject date = dates[idx].toObject();
            int dateId = date[STATE_ID].toInt();
            emit splitDateRequested(mEvent[STATE_ID].toInt(), dateId);
        }
    }
}

#pragma mark Layout
void EventPropertiesView::paintEvent(QPaintEvent* e)
{
    Q_UNUSED(e);
    QWidget::paintEvent(e);
    QPainter p(this);
    p.fillRect(rect(), palette().color(QPalette::Background));
    p.fillRect(QRect(0, 0, width(), mToolbarH), QColor(200, 200, 200));
}

void EventPropertiesView::resizeEvent(QResizeEvent* e)
{
    Q_UNUSED(e);
    updateLayout();
}

void EventPropertiesView::updateLayout()
{
    mTopView->setGeometry(0, 0, width(), mToolbarH);
 
    int butPluginWidth = 80;
    int butPluginHeigth = 50;
    
    mEventView->setGeometry(0, mToolbarH, width(), height()-mToolbarH);
    mBoundView->setGeometry(0, mToolbarH, width(), height()-mToolbarH);
    
    QRect listRect(0, 0, mEventView->width() - butPluginWidth, mEventView->height() - butPluginHeigth);
    mDatesList->setGeometry(listRect);
    
    int x = listRect.width();
    int y = listRect.y();
    
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
    
    x = listRect.x();
    y = listRect.y() + listRect.height();
    int w = listRect.width() / 5;
    int h = butPluginHeigth;

    mCalibBut->setGeometry(x, y, w, h);
    mDeleteBut ->setGeometry(x + 1*w, y, w, h);
    mRecycleBut->setGeometry(x + 2*w, y, w, h);
    mMergeBut->setGeometry(x + 3*w, y, w, h);
    mSplitBut->setGeometry(x + 4*w, y, w, h);
 
    // Known view : Used with Bound
    
    /*y = 0;//mMethodCombo->y() + talonLabel;;
    QRect r = this->rect();
    int m = 5;
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
    
    mKnownGraph->setGeometry(m, y += (lineH + m), r.width() - 2*m, 100);*/
    
    
/* qDebug()<<"EventPropertiesView::sortie update width"<<width()<<" ; height"<<height();
     qDebug()<<"EventPropertiesView::sortie constructeur mEventView width"<<mEventView-> width()<<" ; height"<<mEventView-> height();
   // update(); */
}

