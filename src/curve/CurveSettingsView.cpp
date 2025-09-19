/* ---------------------------------------------------------------------

Copyright or © or Copr. CNRS	2020 - 2025

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

#include "CurveSettingsView.h"

#include "Project.h"
#include "Painting.h"
#include "QtUtilities.h"
#include "StateKeys.h"

#include <QVariant>
#include <QLabel>
#include <QComboBox>
#include <QLineEdit>
#include <QCheckBox>
#include <QGridLayout>
#include <QStyleFactory>
#include <QtWidgets/qabstractitemview.h>
#include <QMessageBox>


CurveSettingsView::CurveSettingsView(QWidget* parent):QWidget(parent)
{
    mTitleLabel = new QLabel(tr("Curve Parameters"), this);
    mTitleLabel->setAlignment(Qt::AlignCenter);
    QFont titleFont;
    titleFont.setBold(true);
    titleFont.setPointSize(20);
    mTitleLabel->setFont(titleFont);
    QPalette palette = mTitleLabel->palette();
    palette.setColor(QPalette::WindowText, CURVE_COLOR_TEXT);
    mTitleLabel->setPalette(palette);

    mDescriptionLabel = new QLabel(tr("These parameters give you controls over the way curves are built. MCMC (Bayesian) can be activated for event time, VG or global smoothing spline factor, etc. "), this);
    mDescriptionLabel->setAlignment(Qt::AlignCenter);
    mDescriptionLabel->setWordWrap(true);

    palette = mDescriptionLabel->palette();
    palette.setColor(QPalette::WindowText, Qt::gray);
    mDescriptionLabel->setPalette(palette);

    mProcessTypeLabel = new QLabel(tr("Process") , this);
    mProcessTypeInput = new QComboBox(this);

    // Ajout des items avec leur valeur enum associée
    mProcessTypeInput->addItem(tr("None"));
    mProcessTypeInput->setItemData(0, static_cast<int>(CurveSettings::eProcess_None));

    mProcessTypeInput->addItem(tr("Univariate (1D)"));
    mProcessTypeInput->setItemData(1, static_cast<int>(CurveSettings::eProcess_Univariate));

    mProcessTypeInput->addItem(tr("Bi-variate (2D)"));
    mProcessTypeInput->setItemData(2, static_cast<int>(CurveSettings::eProcess_2D));

    mProcessTypeInput->addItem(tr("Tri-variate (3D)"));
    mProcessTypeInput->setItemData(3, static_cast<int>(CurveSettings::eProcess_3D));

    // SÉPARATEUR - on peut le garder !
    mProcessTypeInput->insertSeparator(4);

    mProcessTypeInput->addItem(tr("Depth"));
    mProcessTypeInput->setItemData(5, static_cast<int>(CurveSettings::eProcess_Depth));

    // SÉPARATEUR - on peut le garder !
    mProcessTypeInput->insertSeparator(6);

    mProcessTypeInput->addItem(tr("Inclination"));
    mProcessTypeInput->setItemData(7, static_cast<int>(CurveSettings::eProcess_Inclination));

    mProcessTypeInput->addItem(tr("Declination"));
    mProcessTypeInput->setItemData(8, static_cast<int>(CurveSettings::eProcess_Declination));

    mProcessTypeInput->addItem(tr("Field Intensity"));
    mProcessTypeInput->setItemData(9, static_cast<int>(CurveSettings::eProcess_Field));

    mProcessTypeInput->addItem(tr("Spherical (I, D)"));
    mProcessTypeInput->setItemData(10, static_cast<int>(CurveSettings::eProcess_Spherical));

    mProcessTypeInput->addItem(tr("Unknown Dec (I, F)"));
    mProcessTypeInput->setItemData(11, static_cast<int>(CurveSettings::eProcess_Unknwon_Dec));

    mProcessTypeInput->addItem(tr("Vector (I, D, F)"));
    mProcessTypeInput->setItemData(12, static_cast<int>(CurveSettings::eProcess_Vector));


    mThresholdLabel = new QLabel(tr("Minimal Rate of Change"), this);
    mThresholdInput = new QLineEdit(this);

    mUseErrMesureLabel = new QLabel(tr("Use Measurement Err."), this);
    mUseErrMesureInput = new QCheckBox(this);

    mTimeTypeLabel = new QLabel(tr("Event Date"), this);
    mTimeTypeInput = new QComboBox(this);
    mTimeTypeInput->addItem(tr("Fixed"));
    mTimeTypeInput->addItem(tr("Bayesian"));

    mLambdaSplineTypeLabel = new QLabel(tr("Smoothing"), this);
    mLambdaSplineTypeInput = new QComboBox(this);
    mLambdaSplineTypeInput->addItem(tr("Fixed"));
    mLambdaSplineTypeInput->addItem(tr("Bayesian"));
    mLambdaSplineTypeInput->addItem(tr("Interpolation (λ=0)"));

    mLambdaSplineLabel = new QLabel(tr("log 10 Smoothing Value"), this);
    mLambdaSplineInput = new QLineEdit(this);
    mLambdaSplineInput->setText("-6");

    mVarianceTypeLabel = new QLabel(tr("Curve variance (Std gi)"), this);
    mVarianceTypeInput = new QComboBox(this);

    mVarianceTypeInput->addItem(tr("Ind. Bayesian"));
    mVarianceTypeInput->addItem(tr("Global Bayesian"));
    mVarianceTypeInput->addItem(tr("Global Fixed"));

    mVarianceValueLabel = new QLabel(tr("Std gi = Global Value"), this);
    mVarianceValueInput = new QLineEdit(this);

    QGridLayout* grid = new QGridLayout();
    grid->setContentsMargins(0, 0, 0, 0);
    grid->setHorizontalSpacing(10);
    grid->setVerticalSpacing(5);
    int row = -1;

    grid->addWidget(mProcessTypeLabel, ++row, 0, Qt::AlignRight | Qt::AlignVCenter);
    grid->addWidget(mProcessTypeInput, row, 1);

    grid->addWidget(mThresholdLabel, ++row, 0, Qt::AlignRight | Qt::AlignVCenter);
    grid->addWidget(mThresholdInput, row, 1);

    grid->setVerticalSpacing(15);
    grid->addWidget(mUseErrMesureLabel, ++row, 0, Qt::AlignRight | Qt::AlignVCenter);
    grid->addWidget(mUseErrMesureInput, row, 1);

    grid->addWidget(mTimeTypeLabel, ++row, 0, Qt::AlignRight | Qt::AlignVCenter);
    grid->addWidget(mTimeTypeInput, row, 1);

    grid->addWidget(mLambdaSplineTypeLabel, ++row, 0, Qt::AlignRight | Qt::AlignVCenter);
    grid->addWidget(mLambdaSplineTypeInput, row, 1);

    grid->addWidget(mLambdaSplineLabel, ++row, 0, Qt::AlignRight | Qt::AlignVCenter);
    grid->addWidget(mLambdaSplineInput, row, 1);

    grid->addWidget(mVarianceTypeLabel, ++row, 0, Qt::AlignRight | Qt::AlignVCenter);
    grid->addWidget(mVarianceTypeInput, row, 1);

    grid->addWidget(mVarianceValueLabel, ++row, 0, Qt::AlignRight | Qt::AlignVCenter);
    grid->addWidget(mVarianceValueInput, row, 1);


    QVBoxLayout* vlayout = new QVBoxLayout();
    vlayout->addSpacing(20);
    vlayout->addWidget(mTitleLabel);
    vlayout->addSpacing(20);
    vlayout->addWidget(mDescriptionLabel);
    vlayout->addSpacing(30);
    vlayout->addLayout(grid);
    vlayout->addStretch();

    QWidget* vlayoutWidget = new QWidget();
    vlayoutWidget->setFixedWidth(400);
    vlayoutWidget->setLayout(vlayout);

    QHBoxLayout* hlayout = new QHBoxLayout();
    hlayout->setContentsMargins(0, 0, 0, 0);
    hlayout->setSpacing(0);
    hlayout->addStretch();
    hlayout->addWidget(vlayoutWidget);
    hlayout->addStretch();

    setLayout(hlayout);
}

CurveSettingsView::~CurveSettingsView()
{

}

void CurveSettingsView::setConnections(const bool doConnections)
{
    if (doConnections) {
        // updateVisibilities
        connect(mProcessTypeInput, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &CurveSettingsView::updateVisibilities);
        connect(mLambdaSplineTypeInput, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &CurveSettingsView::updateVisibilities);
        connect(mVarianceTypeInput, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &CurveSettingsView::updateVisibilities);

        // Save
        connect(mProcessTypeInput, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &CurveSettingsView::save);
        connect(mThresholdInput, static_cast<void (QLineEdit::*)(const QString&)>(&QLineEdit::textChanged), this, &CurveSettingsView::save);
        connect(mUseErrMesureInput, static_cast<void (QCheckBox::*)(bool)>(&QCheckBox::toggled), this, &CurveSettingsView::save);
        connect(mTimeTypeInput, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &CurveSettingsView::save);
        connect(mVarianceTypeInput, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &CurveSettingsView::save);
        connect(mVarianceValueInput, static_cast<void (QLineEdit::*)(const QString&)>(&QLineEdit::textChanged), this, &CurveSettingsView::save);
        connect(mLambdaSplineTypeInput, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &CurveSettingsView::save);
        connect(mLambdaSplineInput, static_cast<void (QLineEdit::*)(const QString&)>(&QLineEdit::textChanged), this, &CurveSettingsView::save);

    } else {
        // updateVisibilities
        disconnect(mProcessTypeInput, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &CurveSettingsView::updateVisibilities);
        disconnect(mLambdaSplineTypeInput, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &CurveSettingsView::updateVisibilities);
        disconnect(mVarianceTypeInput, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &CurveSettingsView::updateVisibilities);

        // Save
        disconnect(mProcessTypeInput, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &CurveSettingsView::save);
        disconnect(mThresholdInput, static_cast<void (QLineEdit::*)(const QString&)>(&QLineEdit::textChanged), this, &CurveSettingsView::save);
        disconnect(mUseErrMesureInput, static_cast<void (QCheckBox::*)(bool)>(&QCheckBox::toggled), this, &CurveSettingsView::save);
        disconnect(mTimeTypeInput, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &CurveSettingsView::save);
        disconnect(mVarianceTypeInput, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &CurveSettingsView::save);
        disconnect(mVarianceValueInput, static_cast<void (QLineEdit::*)(const QString&)>(&QLineEdit::textChanged), this, &CurveSettingsView::save);
        disconnect(mLambdaSplineTypeInput, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &CurveSettingsView::save);
        disconnect(mLambdaSplineInput, static_cast<void (QLineEdit::*)(const QString&)>(&QLineEdit::textChanged), this, &CurveSettingsView::save);
    }
}

// Fonction utilitaire pour valider et convertir les valeurs numériques
bool CurveSettingsView::validateAndConvertDouble(const QString& text, double& result, const QString& fieldName) const
{
    bool ok;
    result = QLocale().toDouble(text, &ok);
    if (!ok) {
        QMessageBox::warning(const_cast<CurveSettingsView*>(this),
                             tr("Invalid Input"),
                             tr("Invalid value for %1: '%2'").arg(fieldName, text));
        return false;
    }
    return true;
}

void CurveSettingsView::setSettings(const CurveSettings &settings)
{

    // Cherche l'item qui contient la bonne valeur enum
    for (int i = 0; i < mProcessTypeInput->count(); ++i) {
        if (mProcessTypeInput->itemData(i).toInt() == static_cast<int>(settings.mProcessType)) {
            mProcessTypeInput->setCurrentIndex(i);
            break;
        }
    }

    mUseErrMesureInput->setChecked(settings.mUseErrMesure);

    // Mapping pour TimeType avec valeur par défaut explicite
    int timeTypeIndex = 1; // Bayesian par défaut
    if (settings.mTimeType == CurveSettings::eModeFixed) {
        timeTypeIndex = 0;
    } else if (settings.mTimeType == CurveSettings::eModeBayesian) {
        timeTypeIndex = 1;
    }
    mTimeTypeInput->setCurrentIndex(timeTypeIndex);

    // Mapping pour VarianceType avec valeur par défaut explicite
    int varianceTypeIndex = 0; // Ind. Bayesian par défaut
    if (settings.mVarianceType == CurveSettings::eModeBayesian) {
        varianceTypeIndex = 0;
    } else if (settings.mVarianceType == CurveSettings::eModeGlobal) {
        varianceTypeIndex = 1;
    } else if (settings.mVarianceType == CurveSettings::eModeFixed) {
        varianceTypeIndex = 2;
    }
    mVarianceTypeInput->setCurrentIndex(varianceTypeIndex);

    mThresholdInput->setText(stringForLocal(settings.mThreshold));
    mVarianceValueInput->setText(stringForLocal(sqrt(settings.mVarianceFixed)));

    // Mapping pour LambdaSplineType avec valeur par défaut explicite
    int lambdaSplineTypeIndex = 1; // Bayesian par défaut
    if (settings.mLambdaSplineType == CurveSettings::eModeFixed) {
        lambdaSplineTypeIndex = 0;
        mLambdaSplineInput->setText(stringForLocal(log10(settings.mLambdaSpline)));
    } else if (settings.mLambdaSplineType == CurveSettings::eModeBayesian) {
        lambdaSplineTypeIndex = 1;
    } else if (settings.mLambdaSplineType == CurveSettings::eInterpolation) {
        lambdaSplineTypeIndex = 2;
        mLambdaSplineInput->setText("0");
    }
    mLambdaSplineTypeInput->setCurrentIndex(lambdaSplineTypeIndex);

    updateVisibilities();
}

CurveSettings CurveSettingsView::getSettings() const
{
    CurveSettings settings;

    // Récupère directement la valeur enum stockée
    QVariant data = mProcessTypeInput->currentData();
    if (data.isValid()) {
        settings.mProcessType = static_cast<CurveSettings::ProcessType>(data.toInt());
    } else {
        settings.mProcessType = CurveSettings::eProcess_None; // défaut
    }

    settings.mUseErrMesure = mUseErrMesureInput->isChecked();

    // Mapping pour TimeType avec gestion des valeurs par défaut
    switch (mTimeTypeInput->currentIndex()) {
    case 0:
        settings.mTimeType = CurveSettings::eModeFixed;
        break;
    case 1:
    default:
        settings.mTimeType = CurveSettings::eModeBayesian;
        break;
    }

    // Mapping pour VarianceType avec gestion des valeurs par défaut
    switch (mVarianceTypeInput->currentIndex()) {
    case 0:
        settings.mVarianceType = CurveSettings::eModeBayesian;
        break;
    case 1:
        settings.mVarianceType = CurveSettings::eModeGlobal;
        break;
    case 2:
        settings.mVarianceType = CurveSettings::eModeFixed;
        break;
    default:
        settings.mVarianceType = CurveSettings::eModeBayesian;
        break;
    }

    // Validation des valeurs numériques avec gestion d'erreur
    double thresholdValue = 0.0;
    if (validateAndConvertDouble(mThresholdInput->text(), thresholdValue, tr("Threshold"))) {
        settings.mThreshold = thresholdValue;
    } else {
        settings.mThreshold = 0.0; // valeur par défaut en cas d'erreur
    }

    double varianceValue = 1.0;
    if (validateAndConvertDouble(mVarianceValueInput->text(), varianceValue, tr("Variance Value"))) {
        settings.mVarianceFixed = pow(varianceValue, 2.0);
    } else {
        settings.mVarianceFixed = 1.0; // valeur par défaut en cas d'erreur
    }

    // Mapping pour LambdaSplineType avec validation
    switch (mLambdaSplineTypeInput->currentIndex()) {
    case 0: // Fixed
        settings.mLambdaSplineType = CurveSettings::eModeFixed;
        {
            double lambdaValue = -6.0;
            if (validateAndConvertDouble(mLambdaSplineInput->text(), lambdaValue, tr("Lambda Spline"))) {
                if (lambdaValue > 10.0) {
                    settings.mLambdaSpline = pow(10.0, 10.0);
                } else {
                    settings.mLambdaSpline = pow(10.0, lambdaValue);
                }
            } else {
                settings.mLambdaSpline = pow(10.0, -6.0); // valeur par défaut
            }
        }
        break;
    case 2: // Interpolation
        settings.mLambdaSplineType = CurveSettings::eInterpolation;
        settings.mLambdaSpline = 0.0;
        break;
    case 1: // Bayesian
    default:
        settings.mLambdaSplineType = CurveSettings::eModeBayesian;
        break;
    }

    return settings;
}

void CurveSettingsView::setProject()
{
    auto project = getProject_ptr();
    if (project != nullptr)
        setConnections(false);

    const CurveSettings curveSettings(project->mState.value(STATE_CURVE).toObject());

    setSettings(curveSettings);

    setConnections(true);
}

void CurveSettingsView::reset()
{
    CurveSettings settings = CurveSettings::getDefault();
    setSettings(settings);
}

void CurveSettingsView::save()
{
    auto project = getProject_ptr();
    QJsonObject stateNext = project->mState;
    stateNext[STATE_CURVE] = getSettings().toJson();
    project->pushProjectState(stateNext, CURVE_SETTINGS_UPDATED_REASON, true);
    emit newProcess(mProcessTypeInput->currentText());
}

void CurveSettingsView::updateVisibilities()
{
    // Récupère directement l'enum au lieu de l'index
    CurveSettings::ProcessType currentProcess = static_cast<CurveSettings::ProcessType>(
        mProcessTypeInput->currentData().toInt()
        );

    const bool isProcessNone = (currentProcess == CurveSettings::eProcess_None);
    const bool isProcessDepth = (currentProcess == CurveSettings::eProcess_Depth);

    const bool isLambdaSplineFixed = (mLambdaSplineTypeInput->currentIndex() == 0);
    const bool isLambdaSplineInterpol = (mLambdaSplineTypeInput->currentIndex() == 2);
    const bool isVarianceFixed = (mVarianceTypeInput->currentIndex() == 2);

    // Visibilité des contrôles de base
    mUseErrMesureLabel->setVisible(!isProcessNone);
    mUseErrMesureInput->setVisible(!isProcessNone);
    mTimeTypeLabel->setVisible(!isProcessNone);
    mTimeTypeInput->setVisible(!isProcessNone);

    // Threshold seulement pour Depth
    mThresholdLabel->setVisible(isProcessDepth);
    mThresholdInput->setVisible(isProcessDepth);

    // Lambda spline
    mLambdaSplineTypeLabel->setVisible(!isProcessNone);
    mLambdaSplineTypeInput->setVisible(!isProcessNone);
    mLambdaSplineLabel->setVisible(!isProcessNone && isLambdaSplineFixed);
    mLambdaSplineInput->setVisible(!isProcessNone && isLambdaSplineFixed);

    // Variance
    mVarianceTypeLabel->setVisible(!isProcessNone && !isLambdaSplineInterpol);
    mVarianceTypeInput->setVisible(!isProcessNone && !isLambdaSplineInterpol);
    mVarianceValueLabel->setVisible(!isProcessNone && !isLambdaSplineInterpol && isVarianceFixed);
    mVarianceValueInput->setVisible(!isProcessNone && !isLambdaSplineInterpol && isVarianceFixed);
}
