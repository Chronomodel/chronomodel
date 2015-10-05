#include "DateDialog.h"
#include "../PluginFormAbstract.h"
#include "Collapsible.h"
#include "RadioButton.h"
#include "Button.h"
#include "Label.h"
#include "HelpWidget.h"
#include "LineEdit.h"
#include "Painting.h"
#include "ModelUtilities.h"
#include "PluginManager.h"
#include "../PluginAbstract.h"
#include <QtWidgets>


DateDialog::DateDialog(QWidget* parent, Qt::WindowFlags flags):QDialog(parent, flags),
mForm(0),
mWidth(600),
mMargin(5),
mLineH(20),
mButW(80),
mButH(25),
mWiggleEnabled(false)
{
    setWindowTitle(tr("Create / Modify Data"));
    setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum));
    
    // -----------
    
    mNameLab = new QLabel(tr("Name") + " :", this);
    mNameEdit = new QLineEdit(this);
    mNameEdit->setText("<New Date>");
    mNameEdit->QWidget::setStyleSheet("QLineEdit { border-radius: 5px; }");
    mNameEdit->selectAll();
    mNameEdit->setFocus();
    
    QGridLayout* grid = new QGridLayout();
    grid->setContentsMargins(0, 0, 0, 0);
    grid->addWidget(mNameLab, 0, 0, Qt::AlignRight | Qt::AlignVCenter);
    grid->addWidget(mNameEdit, 0, 1);
    
    // ----------
    
    mAdvancedCheck = new QCheckBox(tr("Advanced"));
    mAdvancedWidget = new QGroupBox();
    mAdvancedWidget->setCheckable(false);
    mAdvancedWidget->setVisible(false);
    mAdvancedWidget->setFlat(true);
    connect(mAdvancedCheck, SIGNAL(toggled(bool)), this, SLOT(setAdvancedVisible(bool)));
    
    mMethodLab = new QLabel(tr("Method") + " :", mAdvancedWidget);
    mMethodCombo = new QComboBox(mAdvancedWidget);
    mMethodCombo->addItem(ModelUtilities::getDataMethodText(Date::eMHSymetric));
    mMethodCombo->addItem(ModelUtilities::getDataMethodText(Date::eInversion));
    mMethodCombo->addItem(ModelUtilities::getDataMethodText(Date::eMHSymGaussAdapt));
    
    mWiggleLab = new QLabel(tr("Wiggle Matching") + " :", mAdvancedWidget);
    
    mDeltaFixedRadio = new QRadioButton(tr("Fixed"), mAdvancedWidget);
    mDeltaRangeRadio = new QRadioButton(tr("Range"), mAdvancedWidget);
    mDeltaGaussRadio = new QRadioButton(tr("Gaussian"), mAdvancedWidget);
    mDeltaFixedRadio->setChecked(true);
    
    connect(mDeltaFixedRadio, SIGNAL(toggled(bool)), this, SLOT(updateVisibleControls()));
    connect(mDeltaRangeRadio, SIGNAL(toggled(bool)), this, SLOT(updateVisibleControls()));
    connect(mDeltaGaussRadio, SIGNAL(toggled(bool)), this, SLOT(updateVisibleControls()));
    
    mDeltaHelp = new HelpWidget(tr("Wiggle Sign : \"+\" if data ≤ event, \"-\" if data ≥ event"), mAdvancedWidget);
    mDeltaHelp->setFixedHeight(35);
    mDeltaHelp->setLink("http://www.chronomodel.fr/Chronomodel_User_Manual.pdf#page=11");
    
    mDeltaFixedLab   = new QLabel(tr("Value") + " :", mAdvancedWidget);
    mDeltaMinLab     = new QLabel(tr("Min") + " :", mAdvancedWidget);
    mDeltaMaxLab     = new QLabel(tr("Max") + " :", mAdvancedWidget);
    mDeltaAverageLab = new QLabel(tr("Mean") + " :", mAdvancedWidget);
    mDeltaErrorLab   = new QLabel(tr("Error (sd)") + " :", mAdvancedWidget);
    
    mDeltaFixedEdit   = new QLineEdit(mAdvancedWidget);
    mDeltaMinEdit     = new QLineEdit(mAdvancedWidget);
    mDeltaMaxEdit     = new QLineEdit(mAdvancedWidget);
    mDeltaAverageEdit = new QLineEdit(mAdvancedWidget);
    mDeltaErrorEdit   = new QLineEdit(mAdvancedWidget);
    
    mDeltaFixedEdit->setText(QString::number(0));
    
    
    QGridLayout* advGrid = new QGridLayout();
    advGrid->setContentsMargins(0, 0, 0, 0);
    advGrid->addWidget(mMethodLab, 0, 0, Qt::AlignRight | Qt::AlignVCenter);
    advGrid->addWidget(mMethodCombo, 0, 1);
    advGrid->addWidget(mWiggleLab, 1, 0, Qt::AlignRight | Qt::AlignVCenter);
    advGrid->addWidget(mDeltaFixedRadio, 1, 1);
    advGrid->addWidget(mDeltaRangeRadio, 2, 1);
    advGrid->addWidget(mDeltaGaussRadio, 3, 1);
    advGrid->addWidget(mDeltaHelp, 4, 1);
    
    advGrid->addWidget(mDeltaFixedLab, 5, 0, Qt::AlignRight | Qt::AlignVCenter);
    advGrid->addWidget(mDeltaFixedEdit, 5, 1);
    advGrid->addWidget(mDeltaMinLab, 5, 0, Qt::AlignRight | Qt::AlignVCenter);
    advGrid->addWidget(mDeltaMinEdit, 5, 1);
    advGrid->addWidget(mDeltaMaxLab, 6, 0, Qt::AlignRight | Qt::AlignVCenter);
    advGrid->addWidget(mDeltaMaxEdit, 6, 1);
    advGrid->addWidget(mDeltaAverageLab, 5, 0, Qt::AlignRight | Qt::AlignVCenter);
    advGrid->addWidget(mDeltaAverageEdit, 5, 1);
    advGrid->addWidget(mDeltaErrorLab, 6, 0, Qt::AlignRight | Qt::AlignVCenter);
    advGrid->addWidget(mDeltaErrorEdit, 6, 1);

    mAdvancedWidget->setLayout(advGrid);
    
    // ----------
    
    mButtonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(mButtonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(mButtonBox, SIGNAL(rejected()), this, SLOT(reject()));
    
    mLayout = new QVBoxLayout();
    mLayout->addLayout(grid);
    
    QFrame* line1 = new QFrame();
    line1->setFrameShape(QFrame::HLine);
    line1->setFrameShadow(QFrame::Sunken);
    mLayout->addWidget(line1);
    
    // Plugin form will be put here when ready!
    
    QFrame* line2 = new QFrame();
    line2->setFrameShape(QFrame::HLine);
    line2->setFrameShadow(QFrame::Sunken);
    mLayout->addWidget(line2);
    
    mLayout->addWidget(mAdvancedCheck);
    mLayout->addWidget(mAdvancedWidget);
    mLayout->addWidget(mButtonBox);
    mLayout->addStretch();
    setLayout(mLayout);
    
    setFixedWidth(600);
    
    updateVisibleControls();
}

DateDialog::~DateDialog()
{
    
}

void DateDialog::setForm(PluginFormAbstract* form)
{
    if(mForm)
    {
        mForm->setVisible(false);
        mForm->setParent(0);
    }
    if(form)
    {
        mForm = form;
        mLayout->insertWidget(2, mForm);
        PluginAbstract* plugin = form->mPlugin;
        
        // Check if wiggle is allowed by plugin
        setWiggleEnabled(plugin->wiggleAllowed());
        
        // Disable methods forbidden by plugin
        const QStandardItemModel* model = qobject_cast<const QStandardItemModel*>(mMethodCombo->model());
        for(int i=0; i<mMethodCombo->count(); ++i)
        {
            QStandardItem* item = model->item(i);
            bool allowed = plugin->allowedDataMethods().contains((Date::DataMethod)i);
            
            item->setFlags(!allowed ? item->flags() & ~(Qt::ItemIsSelectable|Qt::ItemIsEnabled)
                           : Qt::ItemIsSelectable|Qt::ItemIsEnabled);
            // visually disable by greying out - works only if combobox has been painted already and palette returns the wanted color
            item->setData(!allowed ? mMethodCombo->palette().color(QPalette::Disabled, QPalette::Text)
                          : QVariant(), // clear item data in order to use default color
                          Qt::TextColorRole);
        }
    }
}

void DateDialog::setWiggleEnabled(bool enabled)
{
    mWiggleEnabled = enabled;
    
    mWiggleLab->setVisible(enabled);
    mDeltaHelp->setVisible(enabled);
    
    mDeltaFixedRadio->setVisible(enabled);
    mDeltaRangeRadio->setVisible(enabled);
    mDeltaGaussRadio->setVisible(enabled);
    
    mDeltaFixedLab->setVisible(enabled);
    mDeltaMinLab->setVisible(enabled);
    mDeltaMaxLab->setVisible(enabled);
    mDeltaAverageLab->setVisible(enabled);
    mDeltaErrorLab->setVisible(enabled);
    
    mDeltaFixedEdit->setVisible(enabled);
    mDeltaMinEdit->setVisible(enabled);
    mDeltaMaxEdit->setVisible(enabled);
    mDeltaAverageEdit->setVisible(enabled);
    mDeltaErrorEdit->setVisible(enabled);
    
    adjustSize();
}

void DateDialog::updateVisibleControls()
{
    mDeltaFixedLab->setVisible(mWiggleEnabled && mDeltaFixedRadio->isChecked());
    mDeltaFixedEdit->setVisible(mWiggleEnabled && mDeltaFixedRadio->isChecked());
    
    mDeltaMinLab->setVisible(mWiggleEnabled && mDeltaRangeRadio->isChecked());
    mDeltaMinEdit->setVisible(mWiggleEnabled && mDeltaRangeRadio->isChecked());
    mDeltaMaxLab->setVisible(mWiggleEnabled && mDeltaRangeRadio->isChecked());
    mDeltaMaxEdit->setVisible(mWiggleEnabled && mDeltaRangeRadio->isChecked());
    
    mDeltaAverageLab->setVisible(mWiggleEnabled && mDeltaGaussRadio->isChecked());
    mDeltaAverageEdit->setVisible(mWiggleEnabled && mDeltaGaussRadio->isChecked());
    mDeltaErrorLab->setVisible(mWiggleEnabled && mDeltaGaussRadio->isChecked());
    mDeltaErrorEdit->setVisible(mWiggleEnabled && mDeltaGaussRadio->isChecked());
    
    adjustSize();
}

void DateDialog::setAdvancedVisible(bool visible)
{
    mAdvancedWidget->setVisible(visible);
    if(visible)
        updateVisibleControls();
    else
        adjustSize();
}

void DateDialog::setDataMethod(Date::DataMethod method)
{
    mMethodCombo->setCurrentIndex((int)method);
}

void DateDialog::setDate(const QJsonObject& date)
{
    mNameEdit->setText(date[STATE_NAME].toString());
    mMethodCombo->setCurrentIndex(date[STATE_DATE_METHOD].toInt());
    
    Date::DeltaType deltaType = (Date::DeltaType)date[STATE_DATE_DELTA_TYPE].toInt();
    
    mDeltaFixedRadio->setChecked(deltaType == Date::eDeltaFixed);
    mDeltaRangeRadio->setChecked(deltaType == Date::eDeltaRange);
    mDeltaGaussRadio->setChecked(deltaType == Date::eDeltaGaussian);
    
    mDeltaFixedEdit->setText(QString::number(date[STATE_DATE_DELTA_FIXED].toDouble()));
    mDeltaMinEdit->setText(QString::number(date[STATE_DATE_DELTA_MIN].toDouble()));
    mDeltaMaxEdit->setText(QString::number(date[STATE_DATE_DELTA_MAX].toDouble()));
    mDeltaAverageEdit->setText(QString::number(date[STATE_DATE_DELTA_AVERAGE].toDouble()));
    mDeltaErrorEdit->setText(QString::number(date[STATE_DATE_DELTA_ERROR].toDouble()));
    
    mNameEdit->selectAll();
    mNameEdit->setFocus();
    
    if(mForm)
    {
        mForm->setData(date[STATE_DATE_DATA].toObject(), date[STATE_DATE_SUB_DATES].toArray().size() > 0);
    }
}

QString DateDialog::getName() const {return mNameEdit->text();}
double DateDialog::getDeltaFixed() const {return mDeltaFixedEdit->text().toDouble();}
double DateDialog::getDeltaMin() const {return mDeltaMinEdit->text().toDouble();}
double DateDialog::getDeltaMax() const {return mDeltaMaxEdit->text().toDouble();}
double DateDialog::getDeltaAverage() const {return mDeltaAverageEdit->text().toDouble();}
double DateDialog::getDeltaError() const {return mDeltaErrorEdit->text().toDouble();}

Date::DataMethod DateDialog::getMethod() const
{
    Date::DataMethod method = Date::eMHSymetric;
    if(mMethodCombo->currentIndex() == 1)
        method = Date::eInversion;
    else if(mMethodCombo->currentIndex() == 2)
        method = Date::eMHSymGaussAdapt;
    return method;
}

Date::DeltaType DateDialog::getDeltaType() const
{
    Date::DeltaType type = Date::eDeltaFixed;
    if(mDeltaRangeRadio->isChecked())
        type = Date::eDeltaRange;
    else if(mDeltaGaussRadio->isChecked())
        type = Date::eDeltaGaussian;
    return type;
}

/*void DateDialog::paintEvent(QPaintEvent* e)
{
    Q_UNUSED(e);
    
    QPainter p(this);
    p.fillRect(rect(), QColor(180, 180, 180));
}*/

/*void DateDialog::resizeEvent(QResizeEvent* e)
{
    Q_UNUSED(e);
    updateLayout();
}

void DateDialog::updateLayout()
{
    int m = mMargin;
    int w = width();
    int h = height();
    int w1 = 100;
    int w2 = w - 3*m - w1;
    
    mNameLab->setGeometry(m, m, w1, mLineH);
    mNameEdit->setGeometry(2*m + w1, m, w2, mLineH);
    
    if(mForm)
    {
        mForm->setGeometry(m, 2*m + mLineH, w - 2*m, mForm->height());
        mAdvanced->setGeometry(m, 2*m + mLineH + mForm->height() + m, w - 2*m, mAdvanced->height());
    }
    
    mOkBut->setGeometry(w - 2*m - 2*mButW, h - m - mButH, mButW, mButH);
    mCancelBut->setGeometry(w - m - mButW, h - m - mButH, mButW, mButH);
    
    w2 = w - 5*m - w1;
    
    int i = 1;
    mMethodLab->setGeometry(m, m, w1, mComboH);
    mDeltaHelp->setGeometry(m, 2*m + mComboH, width() - 4*m, mDeltaHelp->heightForWidth(width() - 4*m));
    mDeltaFixedLab->setGeometry(m, 3*m + mComboH + mDeltaHelp->height() + i * (m + mLineH), w1, mLineH); ++i; ++i;
    mDeltaMinLab->setGeometry(m, 3*m + mComboH + mDeltaHelp->height() + i * (m + mLineH), w1, mLineH); ++i;
    mDeltaMaxLab->setGeometry(m, 3*m + mComboH + mDeltaHelp->height() + i * (m + mLineH), w1, mLineH); ++i; ++i;
    mDeltaAverageLab->setGeometry(m, 3*m + mComboH + mDeltaHelp->height() + i * (m + mLineH), w1, mLineH); ++i;
    mDeltaErrorLab->setGeometry(m, 3*m + mComboH + mDeltaHelp->height() + i * (m + mLineH), w1, mLineH); ++i;
    
    i = 0;
    mMethodCombo->setGeometry(2*m + w1, m, w2, mComboH); ++i;
    mDeltaFixedRadio->setGeometry(m, 3*m + mComboH + mDeltaHelp->height(), w - 4*m, mLineH);
    mDeltaFixedEdit->setGeometry(2*m + w1, 3*m + mComboH + mDeltaHelp->height() + i * (m + mLineH), w2, mLineH); ++i;
    mDeltaRangeRadio->setGeometry(m, 3*m + mComboH + mDeltaHelp->height() + i * (m + mLineH), w - 4*m, mLineH); ++i;
    mDeltaMinEdit->setGeometry(2*m + w1, 3*m + mComboH + mDeltaHelp->height() + i * (m + mLineH), w2, mLineH); ++i;
    mDeltaMaxEdit->setGeometry(2*m + w1, 3*m + mComboH + mDeltaHelp->height() + i * (m + mLineH), w2, mLineH); ++i;
    mDeltaGaussRadio->setGeometry(m, 3*m + mComboH + mDeltaHelp->height() + i * (m + mLineH), w - 4*m, mLineH); ++i;
    mDeltaAverageEdit->setGeometry(2*m + w1, 3*m + mComboH + mDeltaHelp->height() + i * (m + mLineH), w2, mLineH); ++i;
    mDeltaErrorEdit->setGeometry(2*m + w1, 3*m + mComboH + mDeltaHelp->height() + i * (m + mLineH), w2, mLineH); ++i;
}*/

/*void DateDialog::adaptSize()
{
    QSize s(mWidth, 4*mMargin + 1*mLineH + mButH + mAdvanced->height());
    if(mForm)
    {
        s.setHeight(s.height() + mMargin + mForm->height());
    }
    resize(s);
    setFixedHeight(s.height());
}*/
