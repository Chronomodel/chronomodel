/* ---------------------------------------------------------------------

Copyright or © or Copr. CNRS	2014 - 2024

Authors :
	Philippe LANOS
	Helori LANOS
 	Philippe DUFRESNE

This software is a computer program whose purpose is to
create chronological models of archeological data using Bayesian statistics.

This software is governed by the CeCILL V2.1 license under French law and
abiding by the rules of distribution of free software.  You can  use,
modify and/ or redistribute the software under the terms of the CeCILL
license as circulated by CEA, CNRS and INRIA at the following URL
"http://www.cecill.info".

As a counterpart to the access to the source code and  rights to copy,
modify and redistribute granted by the license, users are provided only
with a limited warranty  and the software's author,  the holder of the
economic rights,  and the successive licensors  have only  limited
liability.

In this respect, the user's attention is drawn to the risks associated
with loading,  using,  modifying and/or developing or reproducing the
software by the user in light of its specific status of free software,
that may mean  that it is complicated to manipulate,  and  that  also
therefore means  that it is reserved for developers  and  experienced
professionals having in-depth computer knowledge. Users are therefore
encouraged to load and test the software's suitability as regards their
requirements in conditions enabling the security of their systems and/or
data to be ensured and,  more generally, to use and operate it in the
same conditions as regards security.

The fact that you are presently reading this means that you have had
knowledge of the CeCILL V2.1 license and that you accept its terms.
--------------------------------------------------------------------- */

#include "DateDialog.h"

#include "PluginFormAbstract.h"
#include "HelpWidget.h"
#include "PluginAbstract.h"

#include <QtWidgets>

// ---------------------------------------------------------------------
// 1️⃣  Définition du static member
// ---------------------------------------------------------------------
QLocale DateDialog::sLocale = QLocale::system();   // ou QLocale("fr_FR");


DateDialog::DateDialog(QWidget* parent, Qt::WindowFlags flags):QDialog(parent, flags),
    mWiggleIsValid(false),
    mPluginDataAreValid(false),
    mForm(nullptr),
    mWidth(600),
    mMargin(5),
    mLineH(20),
    mButW(80),
    mButH(25),
    mWiggleEnabled(false)
{
    setWindowTitle(tr("Create / Modify Data"));
    setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum));

    // -----
    mValidator_R = new QDoubleValidator(this);
    mValidator_R->setLocale(QLocale());
    // -----------

    mNameLab = new QLabel(tr("Name"), this);
    mNameEdit = new QLineEdit(this);
    mNameEdit->setAlignment(Qt::AlignHCenter);
    mNameEdit->setText("New Data");
    mNameEdit->QWidget::setStyleSheet("QLineEdit { border: 0px; }");
    mNameEdit->selectAll();
    mNameEdit->setFocus();

    QGridLayout* grid = new QGridLayout();
    grid->setContentsMargins(0, 0, 0, 0);
    grid->addWidget(mNameLab, 0, 0, Qt::AlignRight | Qt::AlignVCenter);
    grid->addWidget(mNameEdit, 0, 1);

    // ----------

    mAdvancedCheck = new QCheckBox(tr("Advanced"));
    mAdvancedWidget = new QGroupBox();
    //mAdvancedWidget->setStyleSheet("QGroupBox {     font: bold;     font-size: 23px;     border: 1px solid silver;     border-radius: 6px;     margin-top: 15px; }");
    mAdvancedWidget->setStyleSheet("QGroupBox {border: 0px}");

    mAdvancedWidget->setCheckable(false);
    mAdvancedWidget->setVisible(false);
    mAdvancedWidget->setFlat(false);
    connect(mAdvancedCheck, &QCheckBox::toggled, this, &DateDialog::setAdvancedVisible);

#ifndef FIXEDPRIOR
    mMethodLab = new QLabel(tr("MCMC"), mAdvancedWidget);
    mMethodCombo = new QComboBox(mAdvancedWidget);
    mMethodCombo->addItem(MHVariable::getSamplerProposalText(SamplerProposal::eDatePrior));
    mMethodCombo->addItem(MHVariable::getSamplerProposalText(SamplerProposal::eLikelihood));
    mMethodCombo->addItem(MHVariable::getSamplerProposalText(SamplerProposal::eRWAdaptGauss));
#endif

    mWiggleLab = new QLabel(tr("Wiggle Matching"), mAdvancedWidget);

    mDeltaNoneRadio  = new QRadioButton(tr("None"), mAdvancedWidget);
    mDeltaFixedRadio = new QRadioButton(tr("Fixed"), mAdvancedWidget);
    mDeltaRangeRadio = new QRadioButton(tr("Range"), mAdvancedWidget);
    mDeltaGaussRadio = new QRadioButton(tr("Gaussian"), mAdvancedWidget);
    mDeltaNoneRadio->setChecked(true);

    connect(mDeltaNoneRadio,  &QRadioButton::toggled, this, &DateDialog::updateVisibleControls);
    connect(mDeltaFixedRadio, &QRadioButton::toggled, this, &DateDialog::updateVisibleControls);
    connect(mDeltaRangeRadio, &QRadioButton::toggled, this,  &DateDialog::updateVisibleControls);
    connect(mDeltaGaussRadio, &QRadioButton::toggled, this,  &DateDialog::updateVisibleControls);

    connect(mDeltaFixedRadio, &QRadioButton::toggled, this, &DateDialog::checkWiggle);
    connect(mDeltaRangeRadio, &QRadioButton::toggled, this,  &DateDialog::checkWiggle);
    connect(mDeltaGaussRadio, &QRadioButton::toggled, this,  &DateDialog::checkWiggle);

    mDeltaHelp = new HelpWidget(tr("Wiggle Sign : \n \"+\" if data ≤ event,\n \"-\" if data ≥ event"), mAdvancedWidget);
    mDeltaHelp->setFixedHeight(70);
    mDeltaHelp->setLink("https://chronomodel.com/storage/medias/83_chronomodel_v32_user_manual_2024_05_13_min.pdf#page=22");

    mDeltaFixedLab = new QLabel(tr("Value"), mAdvancedWidget);
    mDeltaMinLab = new QLabel(tr("Min"), mAdvancedWidget);
    mDeltaMaxLab = new QLabel(tr("Max"), mAdvancedWidget);
    mDeltaAverageLab = new QLabel(tr("Mean"), mAdvancedWidget);
    mDeltaErrorLab = new QLabel(tr("Error (sd)"), mAdvancedWidget);

    mDeltaFixedEdit = new QLineEdit(mAdvancedWidget);
    mDeltaFixedEdit->setAlignment(Qt::AlignHCenter);
    mDeltaFixedEdit->setValidator(mValidator_R);

    mDeltaMinEdit = new QLineEdit(mAdvancedWidget);
    mDeltaMinEdit->setAlignment(Qt::AlignHCenter);
    mDeltaMinEdit->setValidator(mValidator_R);

    mDeltaMaxEdit = new QLineEdit(mAdvancedWidget);
    mDeltaMaxEdit->setAlignment(Qt::AlignHCenter);
    mDeltaMaxEdit->setValidator(mValidator_R);

    mDeltaAverageEdit = new QLineEdit(mAdvancedWidget);
    mDeltaAverageEdit->setAlignment(Qt::AlignHCenter);
    mDeltaAverageEdit->setValidator(mValidator_R);

    mDeltaErrorEdit = new QLineEdit(mAdvancedWidget);
    mDeltaErrorEdit->setAlignment(Qt::AlignHCenter);
    mDeltaErrorEdit->setValidator(mValidator_R);

    mDeltaFixedEdit->setText(QString::number(0));
    mDeltaMinEdit->setText(QString::number(0));
    mDeltaMaxEdit->setText(QString::number(0));
    mDeltaAverageEdit->setText(QString::number(0));
    mDeltaErrorEdit->setText(QString::number(0));

 /*   connect(mDeltaFixedEdit, &QLineEdit::textChanged, this, &DateDialog::checkWiggle);
    connect(mDeltaMinEdit, &QLineEdit::textChanged, this, &DateDialog::checkWiggle);
    connect(mDeltaMaxEdit, &QLineEdit::textChanged, this, &DateDialog::checkWiggle);
    connect(mDeltaAverageEdit, &QLineEdit::textChanged, this, &DateDialog::checkWiggle);
    connect(mDeltaErrorEdit, &QLineEdit::textChanged, this, &DateDialog::checkWiggle);
  */



    // -------------------------------------------------
    // 3️⃣  Fonction utilitaire pour le palette
    // -------------------------------------------------
    auto applyPalette = [](QLineEdit *edit, QValidator::State st){
        QPalette p = edit->palette();
        if (st == QValidator::Acceptable) {
            p.setColor(QPalette::Base, QColor(230,255,230));   // vert pâle
        } else if (st == QValidator::Invalid) {
            p.setColor(QPalette::Base, QColor(255,230,230));   // rouge pâle
        } else { // Intermediate
            p.setColor(QPalette::Base, QColor(255,255,255));   // blanc
        }
        edit->setPalette(p);
    };

    // -------------------------------------------------
    // 4️⃣  Connexions (lambda qui utilise de vraies variables)
    // -------------------------------------------------
  /*  auto updatePalette = [this, applyPalette](QLineEdit *edit){
        QString txt = edit->text();
        int pos = 0;
        QValidator::State st = mValidator_R->validate(txt, pos);
        applyPalette(edit, st);
        // on met à jour le bouton OK en même temps
        //setOkEnabled();
    };

    connect(mDeltaFixedEdit,   &QLineEdit::textChanged,   this, [=, this]{ updatePalette(mDeltaFixedEdit);   });
    connect(mDeltaMinEdit,     &QLineEdit::textChanged,   this, [=, this]{ updatePalette(mDeltaMinEdit);     });
    connect(mDeltaMaxEdit,     &QLineEdit::textChanged,   this, [=, this]{ updatePalette(mDeltaMaxEdit);     });
    connect(mDeltaAverageEdit, &QLineEdit::textChanged,   this, [=, this]{ updatePalette(mDeltaAverageEdit); });
    connect(mDeltaErrorEdit,   &QLineEdit::textChanged,   this, [=, this]{ updatePalette(mDeltaErrorEdit);   });
*/

    connect(mDeltaFixedEdit,   &QLineEdit::textChanged,   this, &DateDialog::checkWiggle);
    connect(mDeltaMinEdit,     &QLineEdit::textChanged,   this, &DateDialog::checkWiggle);
    connect(mDeltaMaxEdit,     &QLineEdit::textChanged,   this, &DateDialog::checkWiggle);
    connect(mDeltaAverageEdit, &QLineEdit::textChanged,   this, &DateDialog::checkWiggle);
    connect(mDeltaErrorEdit,   &QLineEdit::textChanged,   this,&DateDialog::checkWiggle);
    // -------------------------------------------------
    // 5️⃣  Les radios déclenchent aussi la mise à jour du bouton OK
    // -------------------------------------------------
    connect(mDeltaFixedRadio,  &QRadioButton::toggled, this, &DateDialog::setOkEnabled);
    connect(mDeltaRangeRadio,  &QRadioButton::toggled, this, &DateDialog::setOkEnabled);
    connect(mDeltaGaussRadio,  &QRadioButton::toggled, this, &DateDialog::setOkEnabled);


    // ----------
    QFormLayout* form = new QFormLayout;
#ifndef FIXEDPRIOR
    form->addRow(mMethodLab,   mMethodCombo);          // (optionnel)
#endif
    // on crée un widget « vide » qui sert uniquement de place‑maintien
    QWidget *empty = new QWidget;
    empty->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    form->addRow(mWiggleLab);          // le label reste dans la colonne 0,
    // le champ vide occupe la colonne 1

    form->addRow(mDeltaHelp);
    form->addRow(mDeltaNoneRadio);                     // les radios sont ajoutées sans label
    form->addRow(mDeltaFixedRadio);
    form->addRow(mDeltaRangeRadio);
    form->addRow(mDeltaGaussRadio);

    form->addRow(mDeltaFixedLab,   mDeltaFixedEdit);
    form->addRow(mDeltaMinLab,     mDeltaMinEdit);
    form->addRow(mDeltaMaxLab,     mDeltaMaxEdit);
    form->addRow(mDeltaAverageLab, mDeltaAverageEdit);
    form->addRow(mDeltaErrorLab,   mDeltaErrorEdit);

    mAdvancedWidget->setLayout(form);


    mButtonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(mButtonBox, &QDialogButtonBox::accepted, this, &DateDialog::accept);
    connect(mButtonBox, &QDialogButtonBox::rejected, this, &DateDialog::reject);

    mLayout = new QVBoxLayout();
    mLayout->setContentsMargins(5, 5, 5, 5);
    mLayout->setSpacing(5);

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

    updateVisibleControls();
}

DateDialog::~DateDialog()
{
    if (mForm) {
        disconnect(mForm, &PluginFormAbstract::OkEnabled, this, &DateDialog::setOkEnabled);
        disconnect(mForm, &PluginFormAbstract::sizeChanged , this, &DateDialog::updateVisibleControls);
    }
}

void DateDialog::setForm(PluginFormAbstract* form)
{
    if (mForm) {
        mForm->setVisible(false);
        mForm->setParent(nullptr);
    }
    if (form) {

        mForm = form;
        connect(mForm, &PluginFormAbstract::OkEnabled, this, &DateDialog::setPluginDataValid);
        connect(mForm, &PluginFormAbstract::sizeChanged , this, &DateDialog::updateVisibleControls);
        mForm->layout()->setContentsMargins(10, 10, 10, 10);
        mForm->setFlat(true);
        mForm->setStyleSheet("QGroupBox {border: 0px}");
        mLayout->insertWidget(2, mForm);

        PluginAbstract* plugin = form->mPlugin;

        setWindowTitle(tr("Create %1 Data").arg(plugin->getName()));
        // Check if wiggle is allowed by plugin
        mWiggleIsValid = true;
        mPluginDataAreValid = true;

        if (plugin->wiggleAllowed())
            setWiggleEnabled(true);

        else
            setWiggleEnabled(false);

#ifndef FIXEDPRIOR
        // Disable methods forbidden by plugin
        const QStandardItemModel* model = qobject_cast<const QStandardItemModel*>(mMethodCombo->model());

        const QList<SamplerProposal> allowMetho = plugin->allowedDataMethods();
        SamplerProposal spTest;
        for (int i=0; i<mMethodCombo->count(); ++i) {
            QStandardItem* item = model->item(i);

            switch (i) {
            case 0 :
                spTest = SamplerProposal::eDatePrior;
                break;
            case 1 :
                spTest = SamplerProposal::eLikelihood;
                break;
            case 2 :
                spTest = SamplerProposal::eRWAdaptGauss;
                break;
            default :
                spTest = SamplerProposal::eLikelihood;
                break;
            }

            const bool allowed = allowMetho.contains(spTest);

            item->setFlags(!allowed ? item->flags() & ~(Qt::ItemIsSelectable|Qt::ItemIsEnabled)
                           : Qt::ItemIsSelectable|Qt::ItemIsEnabled);
            // visually disable by greying out - works only if combobox has been painted already and palette returns the wanted color
            item->setData(!allowed ? mMethodCombo->palette().color(QPalette::Disabled, QPalette::Text)
                          : QVariant(), // clear item data in order to use default color
                          Qt::ForegroundRole);

        }
#endif

    }
}

void DateDialog::setPluginDataValid(bool valid)
{
    mPluginDataAreValid = valid;
    setOkEnabled();
}

/*void DateDialog::checkWiggle()
{
    if (mDeltaFixedRadio->isChecked()) {
        bool ok = mDeltaFixedEdit->hasAcceptableInput();
        bool ok1 = true;
        mDeltaFixedEdit->text().toDouble(&ok1);
        mWiggleIsValid = ok1 && ok;

    } else if (mDeltaRangeRadio->isChecked()) {
        bool ok = mDeltaMinEdit->hasAcceptableInput() &&  mDeltaMaxEdit->hasAcceptableInput();
        bool ok1 = true;
        bool ok2 = true;
        const int dmin = mDeltaMinEdit->text().toDouble(&ok1);
        const int dmax = mDeltaMaxEdit->text().toDouble(&ok2);
        mWiggleIsValid = ( ok && ok1 && ok2 &&  (dmax>dmin) );

    } else if(mDeltaGaussRadio->isChecked()) {
        bool ok = mDeltaAverageEdit->hasAcceptableInput() &&  mDeltaErrorEdit->hasAcceptableInput();
        bool ok1 = true;
        bool ok2 = true;
        //const double a =
        mDeltaAverageEdit->text().toDouble(&ok1);
        const double e = mDeltaErrorEdit->text().toDouble(&ok2);
        mWiggleIsValid = ( ok && ok1 && ok2 &&  (e > 0) );

    } else
        mWiggleIsValid = true;

    setOkEnabled();
}*/
void DateDialog::checkWiggle()
{
    if (mDeltaFixedRadio->isChecked()) {
        mWiggleIsValid = mDeltaFixedEdit->hasAcceptableInput();

    } else if (mDeltaRangeRadio->isChecked()) {
        if (!mDeltaMinEdit->hasAcceptableInput() && ! mDeltaMaxEdit->hasAcceptableInput()) {
            mWiggleIsValid = false;
        } else {

            const double dmin = sLocale.toDouble(mDeltaMinEdit->text());
            const double dmax = sLocale.toDouble(mDeltaMaxEdit->text());
            mWiggleIsValid = dmax > dmin;
        }

    } else if(mDeltaGaussRadio->isChecked()) {

        // 1️⃣  Les deux champs doivent être acceptables
        if (!mDeltaAverageEdit->hasAcceptableInput() || !mDeltaErrorEdit->hasAcceptableInput())
            mWiggleIsValid = false;

        else {
            // 2️⃣  On ne s’intéresse qu’à l’erreur (doit être > 0)
            const double err = sLocale.toDouble(mDeltaErrorEdit->text());
            mWiggleIsValid = err > 0.0;
        }

    } else
        mWiggleIsValid = true;

    setOkEnabled();
}


void DateDialog::setOkEnabled()
{
    mButtonBox->button(QDialogButtonBox::Ok)->setEnabled(mWiggleIsValid && mPluginDataAreValid);
}

void DateDialog::setWiggleEnabled(bool enabled)
{
    mWiggleEnabled = enabled;

    mWiggleLab->setVisible(enabled);
    mDeltaHelp->setVisible(enabled);

    mDeltaNoneRadio->setVisible(enabled);
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
    if (visible)
        updateVisibleControls();
    else
        adjustSize();
}

#ifndef FIXEDPRIOR
void DateDialog::setDataMethod(SamplerProposal sp)
{
    int index;
    switch (sp) {
    case SamplerProposal::eDatePrior :
        index = 0;
        break;
    case SamplerProposal::eLikelihood:
        index = 1;
        break;
    case SamplerProposal::eRWAdaptGauss:
        index = 2;
        break;
    // The following cases are not for data Method
    case SamplerProposal::eFixe:
    case SamplerProposal::eDoubleExp:
    case SamplerProposal::eEventPrior:
    //case SamplerProposal::eRWAdaptGauss:
    default:
        index = -1;
        break;
    }

    mMethodCombo->setCurrentIndex(index);
}
#endif

void DateDialog::setDate(const QJsonObject& date)
{
    mNameEdit->setText(date.value(STATE_NAME).toString());

#ifndef FIXEDPRIOR
    setDataMethod( (SamplerProposal)date.value(STATE_DATE_SAMPLER).toInt());
#endif

    Date::DeltaType deltaType = Date::DeltaType (date.value(STATE_DATE_DELTA_TYPE).toInt());

    mDeltaNoneRadio->setChecked(deltaType == Date::eDeltaNone);
    mDeltaFixedRadio->setChecked(deltaType == Date::eDeltaFixed);
    mDeltaRangeRadio->setChecked(deltaType == Date::eDeltaRange);
    mDeltaGaussRadio->setChecked(deltaType == Date::eDeltaGaussian);

    auto deltaFixe = date.value(STATE_DATE_DELTA_FIXED).toDouble();
    mDeltaFixedEdit->setText(sLocale.toString(date.value(STATE_DATE_DELTA_FIXED).toDouble()));

    auto deltaMin = date.value(STATE_DATE_DELTA_MIN).toDouble();
    mDeltaMinEdit->setText(sLocale.toString(deltaMin));
    auto deltaMax = date.value(STATE_DATE_DELTA_MAX).toDouble();
    mDeltaMaxEdit->setText(sLocale.toString(deltaMax));

    auto deltaAv = date.value(STATE_DATE_DELTA_AVERAGE).toDouble();
    mDeltaAverageEdit->setText(sLocale.toString(deltaAv));
    auto deltaError = date.value(STATE_DATE_DELTA_ERROR).toDouble();
    mDeltaErrorEdit->setText(sLocale.toString(deltaError));

    // if data are in the JSON they must be valid
    mPluginDataAreValid = true;
    mWiggleIsValid = true;
    mAdvancedCheck->setEnabled(date.value(STATE_DATE_ORIGIN).toInt() != Date::eCombination);

    setOkEnabled();
    // open the display panel if there is wiggle parameter
    if ( (mDeltaFixedRadio->isChecked() && deltaFixe != 0.) ||
         (mDeltaRangeRadio->isChecked() && (deltaMin != 0. || deltaMax != 0.) ) ||
         (mDeltaGaussRadio->isChecked() && deltaError > 0) )
    {

        mAdvancedCheck->setChecked(true);
    }

    mNameEdit->selectAll();
    mNameEdit->setFocus();

    if (mForm)
        mForm->setData(date.value(STATE_DATE_DATA).toObject(), date.value(STATE_DATE_SUB_DATES).toArray().size() > 0);

}


#ifndef FIXEDPRIOR
SamplerProposal DateDialog::getMethod() const
{
    SamplerProposal sampler = SamplerProposal::eDatePrior;
    if (mMethodCombo->currentIndex() == 1)
        sampler = SamplerProposal::eLikelihood;

    else if (mMethodCombo->currentIndex() == 2)
        sampler = SamplerProposal::eRWAdaptGauss;

    return sampler;
}
#endif
Date::DeltaType DateDialog::getDeltaType() const
{
    if (!mWiggleIsValid)
        return Date::eDeltaNone;

    else if (mDeltaFixedRadio->isChecked() && sLocale.toDouble(mDeltaFixedEdit->text()) != 0. )
        return Date::eDeltaFixed;

    else if (mDeltaRangeRadio->isChecked() && (sLocale.toDouble(mDeltaMaxEdit->text())- sLocale.toDouble(mDeltaMinEdit->text())) > 0. )
        return Date::eDeltaRange;

    else if (mDeltaGaussRadio->isChecked() && sLocale.toDouble(mDeltaErrorEdit->text()) > 0. )
        return Date::eDeltaGaussian;

    else
        return Date::eDeltaNone;
}


#pragma mark WiggleValidator

WiggleValidator::WiggleValidator(QObject *parent)
    : QValidator(parent)
{
}

/* ----------------------------------------------------------------- */
void WiggleValidator::setRadios(QRadioButton *fixed,
                                QRadioButton *range,
                                QRadioButton *gauss)
{
    mFixedRadio = fixed;
    mRangeRadio = range;
    mGaussRadio = gauss;
}

/* ----------------------------------------------------------------- */
void WiggleValidator::setEdits(QLineEdit *fixedEdit,
                               QLineEdit *minEdit,
                               QLineEdit *maxEdit,
                               QLineEdit *avgEdit,
                               QLineEdit *errEdit)
{
    mFixedEdit = fixedEdit;
    mMinEdit   = minEdit;
    mMaxEdit   = maxEdit;
    mAvgEdit   = avgEdit;
    mErrEdit   = errEdit;
}

/* ----------------------------------------------------------------- */
QValidator::State WiggleValidator::validate(QString &/*input*/, int &/*pos*/) const
{
    // Le validator est appelé à chaque modification d’un des QLineEdit.
    // Nous ne nous intéressons pas à la chaîne « input » (elle provient
    // du champ qui a déclenché l’appel) mais à l’ensemble des champs.

    // Cas « None » ou aucune radio cochée → toujours valide
    if (!mFixedRadio && !mRangeRadio && !mGaussRadio) {
        mLastResult = true;
        return Acceptable;
    }

    // -----------------------------------------------------------------
    //  1️⃣  Fixed
    // -----------------------------------------------------------------
    if (mFixedRadio && mFixedRadio->isChecked()) {
        bool ok = false;
        mFixedEdit->text().toDouble(&ok);
        mLastResult = ok;
        return ok ? Acceptable : Invalid;
    }

    // -----------------------------------------------------------------
    //  2️⃣  Range
    // -----------------------------------------------------------------
    if (mRangeRadio && mRangeRadio->isChecked()) {
        bool okMin = false, okMax = false;
        double dmin = mMinEdit->text().toDouble(&okMin);
        double dmax = mMaxEdit->text().toDouble(&okMax);

        // Si l’un des deux n’est pas encore un nombre → Intermediate
        if (!okMin || !okMax) {
            mLastResult = false;
            return Intermediate;
        }

        // Condition stricte : max > min
        if (dmax > dmin) {
            mLastResult = true;
            return Acceptable;
        }
        mLastResult = false;
        return Invalid;
    }

    // -----------------------------------------------------------------
    //  3️⃣  Gaussian
    // -----------------------------------------------------------------
    if (mGaussRadio && mGaussRadio->isChecked()) {
        bool okMean = false, okErr = false;
        mAvgEdit->text().toDouble(&okMean);
        double err = mErrEdit->text().toDouble(&okErr);

        if (!okMean || !okErr) {
            mLastResult = false;
            return Intermediate;
        }

        if (err > 0.0) {
            mLastResult = true;
            return Acceptable;
        }
        mLastResult = false;
        return Invalid;
    }

    // Aucun mode reconnu → on considère que c’est valide
    mLastResult = true;
    return Acceptable;
}

/* ----------------------------------------------------------------- */
bool WiggleValidator::isWiggleValid() const
{
    // La méthode validate() a déjà mis à jour mLastResult.
    // Si vous appelez isWiggleValid() sans qu’un champ n’ait
    // déclenché validate(), on force une validation rapide.
    if (mLastResult) return true;   // déjà valide → on ne refait rien

    // Sinon on force une validation (sans tenir compte de l’argument « input »)
    QString dummy;
    int dummyPos = 0;
    validate(dummy, dummyPos);
    return mLastResult;
}
