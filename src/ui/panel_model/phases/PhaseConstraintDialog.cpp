#include "PhaseConstraintDialog.h"
#include "PhaseConstraint.h"
#include "ColorPicker.h"
#include "LineEdit.h"
#include <QtWidgets>


PhaseConstraintDialog::PhaseConstraintDialog(QWidget* parent, Qt::WindowFlags flags):QDialog(parent, flags),
mDeleteRequested(false)
{
    setWindowTitle(tr("Phase Constraint"));
    
    // -----------
    
    QLabel* gammaTypeLab = new QLabel(tr("Time Lag") + " :");
    mGammaFixedLab = new QLabel(tr("Time Lag") + " :");
    mGammaMinLab = new QLabel(tr("Time Lag min") + " :");
    mGammaMaxLab = new QLabel(tr("Time Lag Max") + " :");
    
    gammaTypeLab->setAlignment(Qt::AlignVCenter | Qt::AlignRight);
    mGammaFixedLab->setAlignment(Qt::AlignVCenter | Qt::AlignRight);
    mGammaMinLab->setAlignment(Qt::AlignVCenter | Qt::AlignRight);
    mGammaMaxLab->setAlignment(Qt::AlignVCenter | Qt::AlignRight);
    
    mGammaTypeCombo = new QComboBox();
    mGammaTypeCombo->addItem(tr("Unknown"));
    mGammaTypeCombo->addItem(tr("Range"));
    //mGammaTypeCombo->addItem(tr("Known"));
    
    mGammaFixedEdit = new LineEdit();
    mGammaMinEdit = new LineEdit();
    mGammaMaxEdit = new LineEdit();
    
    
    QGridLayout* formLayout = new QGridLayout();
    formLayout->setContentsMargins(0, 0, 0, 0);
    
    formLayout->addWidget(gammaTypeLab, 0, 0);
    formLayout->addWidget(mGammaTypeCombo, 0, 1);
    formLayout->addWidget(mGammaFixedLab, 1, 0);
    formLayout->addWidget(mGammaFixedEdit, 1, 1);
    formLayout->addWidget(mGammaMinLab, 2, 0);
    formLayout->addWidget(mGammaMinEdit, 2, 1);
    formLayout->addWidget(mGammaMaxLab, 3, 0);
    formLayout->addWidget(mGammaMaxEdit, 3, 1);
    
    // ----------
    
    QDialogButtonBox* buttonBox = new QDialogButtonBox();
    buttonBox->addButton(tr("OK"), QDialogButtonBox::AcceptRole);
    buttonBox->addButton(tr("Cancel"), QDialogButtonBox::RejectRole);
    QPushButton* deleteBut = buttonBox->addButton(tr("Delete"), QDialogButtonBox::DestructiveRole);
    
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
    connect(deleteBut, SIGNAL(clicked()), this, SLOT(deleteConstraint()));
    
    // ----------
    
    QFont font;
    font.setWeight(QFont::Bold);
    
    QLabel* titleLab = new QLabel(tr("Phase Constraint"));
    titleLab->setFont(font);
    titleLab->setAlignment(Qt::AlignCenter);
    
    QFrame* separator = new QFrame();
    separator->setFrameShape(QFrame::HLine);
    separator->setFrameShadow(QFrame::Sunken);
    
    QLabel* intro = new QLabel(tr("<p>A <strong>Phase Constraint</strong> can be of 2 types : <strong>Unknown</strong> or <strong>Range</strong>.</p>\
                                  <p>The Unknown type is the most common. It assumes we don't know anything about the time-lag represented by the constraint.</p>\
                                  <p>The Range type allows to give an interval for the constraint time-lag. The 2 Phases will be forced to respect a time-lag included in this range.</p>"));
    
    intro->setTextFormat(Qt::RichText);
    intro->setWordWrap(true);
    
    // ----------
    
    QVBoxLayout* layout = new QVBoxLayout();
    layout->addWidget(titleLab);
    layout->addWidget(intro);
    layout->addWidget(separator);
    layout->addLayout(formLayout);
    layout->addWidget(buttonBox);
    setLayout(layout);
    
    connect(mGammaTypeCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(showAppropriateGammaOptions()));
}

PhaseConstraintDialog::~PhaseConstraintDialog()
{
    
}

void PhaseConstraintDialog::setConstraint(const PhaseConstraint& constraint)
{
    mGammaTypeCombo->setCurrentIndex((int)constraint.mGammaType);
    mGammaFixedEdit->setText(QString::number(constraint.mGammaFixed));
    mGammaMinEdit->setText(QString::number(constraint.mGammaMin));
    mGammaMaxEdit->setText(QString::number(constraint.mGammaMax));
    mPhaseConstraint = constraint;
    showAppropriateGammaOptions();
}

const PhaseConstraint& PhaseConstraintDialog::getConstraint()
{
    mPhaseConstraint.mGammaType = (PhaseConstraint::GammaType) mGammaTypeCombo->currentIndex();
    mPhaseConstraint.mGammaFixed = mGammaFixedEdit->text().toDouble();
    mPhaseConstraint.mGammaMin = mGammaMinEdit->text().toDouble();
    mPhaseConstraint.mGammaMax = mGammaMaxEdit->text().toDouble();
    return mPhaseConstraint;
}

void PhaseConstraintDialog::deleteConstraint()
{
    mDeleteRequested = true;
    accept();
}

void PhaseConstraintDialog::showAppropriateGammaOptions()
{
    PhaseConstraint::GammaType type = (PhaseConstraint::GammaType) mGammaTypeCombo->currentIndex();
    switch(type)
    {
        case PhaseConstraint::eGammaUnknown:
        {
            mGammaFixedLab->setVisible(false);
            mGammaMinLab->setVisible(false);
            mGammaMaxLab->setVisible(false);
            
            mGammaFixedEdit->setVisible(false);
            mGammaMinEdit->setVisible(false);
            mGammaMaxEdit->setVisible(false);
            break;
        }
        case PhaseConstraint::eGammaFixed:
        {
            mGammaFixedLab->setVisible(true);
            mGammaMinLab->setVisible(false);
            mGammaMaxLab->setVisible(false);
            
            mGammaFixedEdit->setVisible(true);
            mGammaMinEdit->setVisible(false);
            mGammaMaxEdit->setVisible(false);
            break;
        }
        case PhaseConstraint::eGammaRange:
        {
            mGammaFixedLab->setVisible(false);
            mGammaMinLab->setVisible(true);
            mGammaMaxLab->setVisible(true);
            
            mGammaFixedEdit->setVisible(false);
            mGammaMinEdit->setVisible(true);
            mGammaMaxEdit->setVisible(true);
            break;
        }
        default:
            break;
    }
}

