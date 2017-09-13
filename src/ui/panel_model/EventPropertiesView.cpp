 #include "EventPropertiesView.h"
#include "Label.h"
#include "LineEdit.h"
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
mButtonWidth(50)
{
    QFontMetrics fm (font());
    minimumHeight = 0;
    mToolbarH = 5 * fm.height();
    // ------------- commun with defautlt Event and Bound ----------
    mNameLab = new Label(tr("Name"), this);
    
    mNameEdit = new LineEdit(this);
    
    mColorLab = new Label(tr("Color"), this);
    mColorPicker = new ColorPicker(Qt::black);
    mColorPicker->setFixedHeight(17);
    
    mMethodLab = new Label(tr("Method"), this);
    mMethodCombo = new QComboBox();
    mMethodCombo->setFixedHeight(20);
    
    mMethodCombo->addItem(ModelUtilities::getEventMethodText(Event::eDoubleExp));
    mMethodCombo->addItem(ModelUtilities::getEventMethodText(Event::eBoxMuller));
    mMethodCombo->addItem(ModelUtilities::getEventMethodText(Event::eMHAdaptGauss));
    
    connect(mNameEdit, &QLineEdit::editingFinished, this, &EventPropertiesView::updateEventName);
    connect(mColorPicker, &ColorPicker::colorChanged, this, &EventPropertiesView::updateEventColor);
    connect(mMethodCombo, static_cast<void (QComboBox::*)(int)>(&QComboBox::activated), this, &EventPropertiesView::updateEventMethod);
    
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
    topLayout->setContentsMargins(6, 6, 6, 6);
    topLayout->addLayout(grid);
    topLayout->addStretch();
    
    mTopView = new QWidget(this);
    mTopView->setLayout(topLayout);
 //   mTopView->setFixedHeight(mToolbarH);
    mTopView->setFixedHeight(mNameEdit->height()+mColorPicker->height()+mMethodCombo->height() + 2*grid->spacing());
      
    // Event default propreties Window mEventView
    mEventView = new QWidget(this);
    
    minimumHeight += mEventView->height();
    // -------------
    
    mDatesList = new DatesList(mEventView);
    connect(mDatesList, &DatesList::calibRequested, this, &EventPropertiesView::updateCalibRequested);

    // -------------

    const QList<PluginAbstract*>& plugins = PluginManager::getPlugins();

    for (int i=0; i<plugins.size(); ++i) {
        
        Button* button = new Button(plugins.at(i)->getName(), mEventView);
        button->setIcon(plugins.at(i)->getIcon());
        button->setFlatVertical();
        button->setIconOnly(false);
        button->setToolTip(tr("Insert") +" "+ plugins.at(i)->getName() +" "+ tr("Data") );
        button->resize(mButtonWidth, mButtonWidth);
        connect(button, SIGNAL(clicked()), this, SLOT(createDate()));
        
        minimumHeight+=button->height();
        
        if (plugins.at(i)->doesCalibration())
            mPluginButs1.append(button);
        else
            mPluginButs2.append(button);
    }
    
    mDeleteBut = new Button(tr("Delete"), mEventView);
    mDeleteBut->setIcon(QIcon(":delete.png"));
    mDeleteBut->setFlatVertical();
    mDeleteBut->setToolTip(tr("Delete Data"));

    connect(mDeleteBut, static_cast<void (QPushButton::*)(bool)>(&Button::clicked), this, &EventPropertiesView::deleteSelectedDates);
    minimumHeight += mDeleteBut->height();
    
    mRecycleBut = new Button(tr("Restore"), mEventView);
    mRecycleBut->setIcon(QIcon(":restore.png"));
    mRecycleBut->setFlatVertical();
    mRecycleBut->setToolTip(tr("Restore Deleted Data"));

    connect(mRecycleBut, static_cast<void (QPushButton::*)(bool)>(&Button::clicked), this, &EventPropertiesView::recycleDates);

    // ---------------
    
    mCalibBut = new Button(tr("Calibrate"), mEventView);
    mCalibBut->setIcon(QIcon(":results_w.png"));
    mCalibBut->setFlatVertical();
    mCalibBut->setCheckable(true);
    mCalibBut->setToolTip(tr("Calibrate"));
    
    mCombineBut = new Button(tr("Combine"), mEventView);
    mCombineBut->setFlatVertical();
    mCombineBut->setIconOnly(false);
    mCombineBut->setEnabled(false);
    mCombineBut->setToolTip(tr("Combine 14C ages"));

    mSplitBut = new Button(tr("Split"), mEventView);
    mSplitBut->setFlatVertical();
    mSplitBut->setIconOnly(false);
    mSplitBut->setEnabled(false);
    mSplitBut->setToolTip(tr("Split combined 14C ages"));

    connect(mCalibBut, &Button::clicked, this, &EventPropertiesView::showCalibRequested);
    connect(mCombineBut, static_cast<void (QPushButton::*)(bool)>(&Button::clicked), this, &EventPropertiesView::sendCombineSelectedDates);
    connect(mSplitBut, static_cast<void (QPushButton::*)(bool)>(&Button::clicked), this, &EventPropertiesView::sendSplitDate);

    // --------------- Case of Event is a Bound -> Bound properties windows---------------------------
    
    mBoundView = new QWidget(this);
    
    //mKnownFixedRadio   = new QRadioButton(tr("Fixed"), mBoundView);
    //mKnownUniformRadio = new QRadioButton(tr("Uniform"), mBoundView);
    
   // connect(mKnownFixedRadio, static_cast<void (QRadioButton::*)(bool)>(&QRadioButton::clicked), this, &EventPropertiesView::updateKnownType);
   // connect(mKnownUniformRadio, static_cast<void (QRadioButton::*)(bool)>(&QRadioButton::clicked), this, &EventPropertiesView::updateKnownType);
    
    mKnownFixedEdit = new QLineEdit(mBoundView);
    //mKnownStartEdit = new QLineEdit(mBoundView);
    //mKnownEndEdit   = new QLineEdit(mBoundView);
    
    QDoubleValidator* doubleValidator = new QDoubleValidator(this);
    doubleValidator->setDecimals(2);
    
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
    
    connect(mDatesList, &DatesList::itemSelectionChanged, this, &EventPropertiesView::updateCombineAvailability);
    connect(mKnownFixedEdit, &QLineEdit::textEdited, this, &EventPropertiesView::updateKnownFixed);
    //connect(mKnownStartEdit, &QLineEdit::textEdited, this, &EventPropertiesView::updateKnownUnifStart);
    //connect(mKnownEndEdit, &QLineEdit::textEdited, this, &EventPropertiesView::updateKnownUnifEnd);
    
    QFormLayout* fixedLayout = new QFormLayout();
    fixedLayout->addRow(QString(tr("Value") + " " + "(BC/AD)"), mKnownFixedEdit);
    mFixedGroup = new QGroupBox();
    mFixedGroup->setLayout(fixedLayout);
    
    /*QFormLayout* uniformLayout = new QFormLayout();
    uniformLayout->addRow(tr("Start"), mKnownStartEdit);
    uniformLayout->addRow(tr("End"), mKnownEndEdit);
    mUniformGroup = new QGroupBox();
    mUniformGroup->setLayout(uniformLayout);
    */
    QVBoxLayout* boundLayout = new QVBoxLayout();
    boundLayout->setContentsMargins(10, 6, 15, 6);
    boundLayout->setSpacing(10);
//  boundLayout->addWidget(mKnownFixedRadio);
    boundLayout->addWidget(mFixedGroup);
//  boundLayout->addWidget(mKnownUniformRadio);
//  boundLayout->addWidget(mUniformGroup);
    boundLayout->addWidget(mKnownGraph);
    boundLayout->addStretch();
    mBoundView->setLayout(boundLayout);
    
    mEvent = QJsonObject();
    mTopView->setVisible(false);
    mEventView->setVisible(false);
    mBoundView->setVisible(false);
}

EventPropertiesView::~EventPropertiesView()
{
    
}

// Event Managment
void EventPropertiesView::setEvent(const QJsonObject& event)
{
    mEvent = event;
    if (rect().width() > 0)
        updateEvent();
}

void EventPropertiesView::updateEvent()
{
    qDebug()<<"EventPropertiesView::updateEvent";

    if (mEvent.isEmpty()) {
        mTopView->setVisible(false);
        mEventView->setVisible(false);
        mBoundView->setVisible(false);
        
    } else {
        Event::Type type = (Event::Type)mEvent.value(STATE_EVENT_TYPE).toInt();
        QString name = mEvent.value(STATE_NAME).toString();
        QColor color(mEvent.value(STATE_COLOR_RED).toInt(),
                     mEvent.value(STATE_COLOR_GREEN).toInt(),
                     mEvent.value(STATE_COLOR_BLUE).toInt());
        
        if (name != mNameEdit->text())
            mNameEdit->setText(name);

        mColorPicker->setColor(color);
        
        mMethodLab->setVisible(type == Event::eDefault);
        mMethodCombo->setVisible(type == Event::eDefault);
        
        mTopView->setVisible(true);
        mEventView->setVisible(type == Event::eDefault);
        mBoundView->setVisible(type == Event::eKnown);
        
        if (type == Event::eDefault) {
            mMethodCombo->setCurrentIndex(mEvent.value(STATE_EVENT_METHOD).toInt());
            mDatesList->setEvent(mEvent);
            mDatesList->setCurrentRow(0);

            QJsonArray dates = mEvent.value(STATE_EVENT_DATES).toArray();

            bool hasDates = (dates.size() > 0);
            if (hasDates)
                updateCalibRequested(dates[0].toObject());

            mCalibBut->setEnabled(hasDates);
            mDeleteBut->setEnabled(hasDates);
            mRecycleBut->setEnabled(true);
            
        } else if (type == Event::eKnown) {
            mKnownFixedEdit -> setText(QString::number(mEvent.value(STATE_EVENT_KNOWN_FIXED).toDouble()));
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


void EventPropertiesView::updateEventName()
{
    QJsonObject event = mEvent;
    event[STATE_NAME] = mNameEdit->text();
    MainWindow::getInstance()->getProject()->updateEvent(event, tr("Event name updated"));
}

void EventPropertiesView::updateEventColor(const QColor &color)
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

//#pragma mark Event Known Properties
/*
void EventPropertiesView::updateKnownType()
{
    if ((Event::Type)mEvent.value(STATE_EVENT_TYPE).toInt() == Event::eKnown)  {
        EventKnown::KnownType type = EventKnown::eFixed;
       // if(mKnownUniformRadio->isChecked())
            type = EventKnown::eUniform;
        
        if ((EventKnown::KnownType)mEvent.value(STATE_EVENT_KNOWN_TYPE).toInt() != type) {
            QJsonObject event = mEvent;
            event[STATE_EVENT_KNOWN_TYPE] = type;
            MainWindow::getInstance()->getProject()->updateEvent(event, tr("Bound type updated"));
        }
    }
}
*/
void EventPropertiesView::updateKnownFixed(const QString& text)
{
    QJsonObject event = mEvent;
    event[STATE_EVENT_KNOWN_FIXED] = round(text.toDouble());
    MainWindow::getInstance()->getProject()->updateEvent(event, tr("Bound fixed value updated"));
}



void EventPropertiesView::updateKnownGraph()
{
    mKnownGraph->removeAllCurves();
    

        Project* project = MainWindow::getInstance()->getProject();
        QJsonObject state = project->state();
        QJsonObject settings = state.value(STATE_SETTINGS).toObject();
        const double tmin = settings.value(STATE_SETTINGS_TMIN).toDouble();
        const double tmax = settings.value(STATE_SETTINGS_TMAX).toDouble();
        const double step = settings.value(STATE_SETTINGS_STEP).toDouble();
        EventKnown event = EventKnown::fromJson(mEvent);

        if ( (tmin>event.mFixed) || (event.mFixed>tmax) ) {

            return;
        }

        event.updateValues(tmin, tmax,step );
        
        mKnownGraph->setRangeX(tmin,tmax);
        
        //qDebug() << "EventPropertiesView::updateKnownGraph()"<<event.mValues.size();
        mKnownGraph->setCurrentX(tmin,tmax);
        
        double max = map_max_value(event.mValues);
        max = (max == 0) ? 1 : max;
        mKnownGraph->setRangeY(0, max);
        mKnownGraph->showYAxisValues(false);

        //---------------------

        GraphCurve curve;
        curve.mName = "Bound";
        curve.mBrush = Painting::mainColorLight;

        curve.mPen = QPen(Painting::mainColorLight, 2.);
        
        curve.mIsHorizontalSections = true;
        qreal tLower;
        qreal tUpper;
        tLower = event.fixedValue();
        tUpper = tLower;


        curve.mSections.append(qMakePair(tLower,tUpper));
        mKnownGraph->addCurve(curve);
        //---------------------
    //}
}

/*
void EventPropertiesView::updateKnownControls()
{
    mFixedGroup->setVisible(mKnownFixedRadio->isChecked());
    mUniformGroup->setVisible(mKnownUniformRadio->isChecked());
}
*/

void EventPropertiesView::createDate()
{
    if (!mEvent.isEmpty()) {
        Button* but = dynamic_cast<Button*>(sender());
        if (but) {
            Project* project = MainWindow::getInstance()->getProject();
            const QList<PluginAbstract*>& plugins = PluginManager::getPlugins();
            
            for (int i=0; i<plugins.size(); ++i) {
                if (plugins.at(i)->getName() == but->text()) {
                    Date date = project->createDateFromPlugin(plugins.at(i));
                    if (!date.isNull())
                        project->addDate(mEvent.value(STATE_ID).toInt(), date.toJson());
                }
            }
        }
    }
}

void EventPropertiesView::deleteSelectedDates()
{
    QList<QListWidgetItem*> items = mDatesList->selectedItems();
    QList<int> indexes;
    for (int i=0; i<items.size(); ++i)
        indexes.push_back(mDatesList->row(items[i]));
    
    MainWindow::getInstance()->getProject()->deleteDates(mEvent.value(STATE_ID).toInt(), indexes);
}

void EventPropertiesView::recycleDates()
{
    MainWindow::getInstance()->getProject()->recycleDates(mEvent.value(STATE_ID).toInt());
}

// Merge / Split
void EventPropertiesView::updateCombineAvailability()
{
    bool mergeable (false);
    bool splittable (false);
    
    QJsonArray dates = mEvent.value(STATE_EVENT_DATES).toArray();
    QList<QListWidgetItem*> items = mDatesList->selectedItems();
    
    if (items.size() == 1) {
        // Split?
        int idx = mDatesList->row(items[0]);
        if (dates.size()>idx ) {
            QJsonObject date = dates.at(idx).toObject();
            if (date.value(STATE_DATE_SUB_DATES).toArray().size() > 0)
                splittable = true;
            
         }
    } else if (items.size() > 1 && dates.size() > 1) {
        // Combine?
        mergeable = true;
        PluginAbstract* plugin = nullptr;
        
        for (int i(0); i<items.size(); ++i) {
            int idx = mDatesList->row(items.at(i));
            if (idx < dates.size()) {
                QJsonObject date = dates.at(idx).toObject();
                // If selected date already has subdates, it cannot be combined :
                if (date.value(STATE_DATE_SUB_DATES).toArray().size() > 0) {
                    mergeable = false;
                    break;
                }
                // If selected dates have different plugins, they cannot be combined :
                PluginAbstract* plg = PluginManager::getPluginFromId(date.value(STATE_DATE_PLUGIN_ID).toString());
                if (plugin == nullptr)
                    plugin = plg;
                else if (plg != plugin) {
                    mergeable = false;
                    break;
                }
            }
        }
        // Dates are not combined yet and are from the same plugin.
        // We should now ask the plugin if they are combinable (they use the same ref curve for example...)
        if (mergeable && plugin != nullptr) {
            // This could be used instead to disable the "combine" button if dates cannot be combined.
            // We prefer letting the user combine them and get an error message explaining why they cannot be combined!
            // Check Plugin14C::mergeDates as example
            //mergeable = plugin->areDatesMergeable(dates);
            
            mergeable = true;
        }
    }
    mCombineBut->setEnabled(mergeable);
    mSplitBut->setEnabled(splittable);
}

void EventPropertiesView::sendCombineSelectedDates()
{
    QJsonArray dates = mEvent.value(STATE_EVENT_DATES).toArray();
    QList<QListWidgetItem*> items = mDatesList->selectedItems();
    QList<int> dateIds;
    
    for (int i=0; i<items.size(); ++i) {
        const int idx = mDatesList->row(items.at(i));
        if (idx < dates.size()) {
            QJsonObject date = dates.at(idx).toObject();
            dateIds.push_back(date.value(STATE_ID).toInt());
        }
    }
    emit combineDatesRequested(mEvent.value(STATE_ID).toInt(), dateIds);
}

void EventPropertiesView::sendSplitDate()
{
    QJsonArray dates = mEvent.value(STATE_EVENT_DATES).toArray();
    QList<QListWidgetItem*> items = mDatesList->selectedItems();
    if (items.size() > 0) {
        int idx = mDatesList->row(items[0]);
        if (idx < dates.size()) {
            QJsonObject date = dates.at(idx).toObject();
            int dateId = date.value(STATE_ID).toInt();
            emit splitDateRequested(mEvent.value(STATE_ID).toInt(), dateId);
        }
    }
}

/**
 * @brief EventPropertiesView::keyPressEvent Append only when the windows has the focus
 * @param event
 */
void EventPropertiesView::keyPressEvent(QKeyEvent* e)
{
    if ((e->key() == Qt::Key_Escape) && mCalibBut->isChecked())
        mCalibBut->click();
    else if ((e->key() == Qt::Key_C) && !mCalibBut->isChecked())
        mCalibBut->click();

    QWidget::keyPressEvent(e);

}

//#pragma mark Layout
void EventPropertiesView::paintEvent(QPaintEvent* e)
{
    Q_UNUSED(e);
    QWidget::paintEvent(e);
    QPainter p(this);
    p.fillRect(rect(), palette().color(QPalette::Background));
    
    if (mEvent.isEmpty()) {
        QFont font = p.font();
        font.setBold(true);
        font.setPointSize(20);
        p.setFont(font);
        p.setPen(QColor(150, 150, 150));
        p.drawText(rect(), Qt::AlignCenter, tr("No event selected !"));
    }

}

void EventPropertiesView::resizeEvent(QResizeEvent* e)
{
    Q_UNUSED(e);
    updateLayout();
}

void EventPropertiesView::updateLayout()
{

    mTopView->setGeometry(0, 0, width(), mToolbarH);

    if (hasEvent()) {
        int butPluginHeigth = mButtonWidth;

        // in EventPropertiesView coordinates
        mBoundView->resize(0, 0);
        mEventView->setGeometry(0, mTopView->height(), width(), height() - mTopView->height());


         //in mEventView coordinates
        QRect listRect(0, 0, mEventView->width() - mButtonWidth, mEventView->height() - butPluginHeigth);
        mDatesList->setGeometry(listRect);

        int x = listRect.width();
        int y = 0;

        // Plugin with calibration,
        //for (int i=0; i<mPluginButs1.size(); ++i) {
        for (auto && plugBut : mPluginButs1) {
            plugBut->setGeometry(x, y, mButtonWidth, butPluginHeigth);
            y += butPluginHeigth;
        }

        // plugin without calibration
        //for (int i(0); i<mPluginButs2.size(); ++i) {
        for (auto && plugBut : mPluginButs2) {
            plugBut->setGeometry(x, y, mButtonWidth, butPluginHeigth);
            y += butPluginHeigth;
        }

        x = listRect.x();
        y = listRect.y() + listRect.height();
        const int w = mButtonWidth; //listRect.width() / 5;
        const int h = mButtonWidth;

        mCalibBut->setGeometry(0, y, w, butPluginHeigth);
        mDeleteBut ->setGeometry(mCalibBut->width(), y, w, h);
        mRecycleBut->setGeometry(mDeleteBut->x() + mDeleteBut->width(), y, w, butPluginHeigth);
        mCombineBut->setGeometry(mRecycleBut->x() + mRecycleBut->width(), y, w, butPluginHeigth);
        mSplitBut->setGeometry(mCombineBut->x() + mCombineBut->width(), y, w, butPluginHeigth);
    }
    else {
        if (hasBound()) {
            mEventView->resize(0, 0);
            mBoundView->setGeometry(0, mTopView->height(), width() - mButtonWidth, height() - mTopView->height());
        }
    }
}

void EventPropertiesView::setCalibChecked(bool checked)
{
    mCalibBut->setChecked(checked);
}

bool EventPropertiesView::isCalibChecked() const
{
    return mCalibBut->isChecked();
}

bool EventPropertiesView::hasEvent() const
{
    if (!mEvent.isEmpty()) {
        Event::Type type = (Event::Type)mEvent[STATE_EVENT_TYPE].toInt();
        return (type == Event::eDefault);
    }
    return false;
}

bool EventPropertiesView::hasBound() const
{
    if (!mEvent.isEmpty()) {
        Event::Type type = (Event::Type)mEvent[STATE_EVENT_TYPE].toInt();
        return (type == Event::eKnown);
    }
    return false;
}

bool EventPropertiesView::hasEventWithDates() const
{
    if (!mEvent.isEmpty()) {
        Event::Type type = (Event::Type)mEvent[STATE_EVENT_TYPE].toInt();
        if (type == Event::eDefault) {
            const QJsonArray dates = mEvent[STATE_EVENT_DATES].toArray();
            return (dates.size() > 0);
        }
    }
    return false;
}
