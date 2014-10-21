#include "EventConstraintDialog.h"
#include "EventConstraint.h"
#include "ColorPicker.h"
#include "LineEdit.h"
#include <QtWidgets>


EventConstraintDialog::EventConstraintDialog(QWidget* parent, Qt::WindowFlags flags):QDialog(parent, flags),
mDeleteRequested(false)
{
    setWindowTitle(tr("Event Constraint"));
    
    // -----------
    
    QLabel* phiTypeLab = new QLabel(tr("Time Lag Type") + " :");
    mPhiFixedLab = new QLabel(tr("Time Lag") + " :");
    mPhiMinLab = new QLabel(tr("Time Lag min") + " :");
    mPhiMaxLab = new QLabel(tr("Time Lag max") + " :");
    
    phiTypeLab->setAlignment(Qt::AlignVCenter | Qt::AlignRight);
    mPhiFixedLab->setAlignment(Qt::AlignVCenter | Qt::AlignRight);
    mPhiMinLab->setAlignment(Qt::AlignVCenter | Qt::AlignRight);
    mPhiMaxLab->setAlignment(Qt::AlignVCenter | Qt::AlignRight);
    
    mPhiTypeCombo = new QComboBox();
    mPhiTypeCombo->addItem(tr("Unknown"));
    mPhiTypeCombo->addItem(tr("Range"));
    //mPhiTypeCombo->addItem(tr("Known"));
    
    mPhiFixedEdit = new LineEdit();
    mPhiMinEdit = new LineEdit();
    mPhiMaxEdit = new LineEdit();
    
    
    QGridLayout* formLayout = new QGridLayout();
    formLayout->setContentsMargins(0, 0, 0, 0);
    
    formLayout->addWidget(phiTypeLab, 0, 0);
    formLayout->addWidget(mPhiTypeCombo, 0, 1);
    formLayout->addWidget(mPhiFixedLab, 1, 0);
    formLayout->addWidget(mPhiFixedEdit, 1, 1);
    formLayout->addWidget(mPhiMinLab, 2, 0);
    formLayout->addWidget(mPhiMinEdit, 2, 1);
    formLayout->addWidget(mPhiMaxLab, 3, 0);
    formLayout->addWidget(mPhiMaxEdit, 3, 1);
    
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
    
    QLabel* titleLab = new QLabel(tr("Event Constraint"));
    titleLab->setFont(font);
    titleLab->setAlignment(Qt::AlignCenter);
    
    QFrame* separator = new QFrame();
    separator->setFrameShape(QFrame::HLine);
    separator->setFrameShadow(QFrame::Sunken);
    
    QLabel* intro = new QLabel(tr("<p>A <strong>Event Constraint</strong> can be of 2 types : <strong>Unknown</strong> or <strong>Range</strong>.</p>\
                                  <p>The Unknown type is the most common. It assumes we don't know anything about the time-lag represented by the constraint.</p>\
                                  <p>The Range type allows to give an interval for the constraint time-lag. The 2 events will be forced to respect a time-lag included in this range.</p>"));
    
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
    
    connect(mPhiTypeCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(showAppropriatePhiOptions()));
}

EventConstraintDialog::~EventConstraintDialog()
{
    
}

void EventConstraintDialog::setConstraint(const EventConstraint& constraint)
{
    mPhiTypeCombo->setCurrentIndex((int)constraint.mPhiType);
    mPhiFixedEdit->setText(QString::number(constraint.mPhiFixed));
    mPhiMinEdit->setText(QString::number(constraint.mPhiMin));
    mPhiMaxEdit->setText(QString::number(constraint.mPhiMax));
    mEventConstraint = constraint;
    showAppropriatePhiOptions();
}

const EventConstraint& EventConstraintDialog::getConstraint()
{
    mEventConstraint.mPhiType = (EventConstraint::PhiType) mPhiTypeCombo->currentIndex();
    mEventConstraint.mPhiFixed = mPhiFixedEdit->text().toDouble();
    mEventConstraint.mPhiMin = mPhiMinEdit->text().toDouble();
    mEventConstraint.mPhiMax = mPhiMaxEdit->text().toDouble();
    return mEventConstraint;
}

void EventConstraintDialog::deleteConstraint()
{
    mDeleteRequested = true;
    accept();
}

void EventConstraintDialog::showAppropriatePhiOptions()
{
    EventConstraint::PhiType type = (EventConstraint::PhiType) mPhiTypeCombo->currentIndex();
    switch(type)
    {
        case EventConstraint::ePhiUnknown:
        {
            mPhiFixedLab->setVisible(false);
            mPhiMinLab->setVisible(false);
            mPhiMaxLab->setVisible(false);
            
            mPhiFixedEdit->setVisible(false);
            mPhiMinEdit->setVisible(false);
            mPhiMaxEdit->setVisible(false);
            break;
        }
        case EventConstraint::ePhiFixed:
        {
            mPhiFixedLab->setVisible(true);
            mPhiMinLab->setVisible(false);
            mPhiMaxLab->setVisible(false);
            
            mPhiFixedEdit->setVisible(true);
            mPhiMinEdit->setVisible(false);
            mPhiMaxEdit->setVisible(false);
            break;
        }
        case EventConstraint::ePhiRange:
        {
            mPhiFixedLab->setVisible(false);
            mPhiMinLab->setVisible(true);
            mPhiMaxLab->setVisible(true);
            
            mPhiFixedEdit->setVisible(false);
            mPhiMinEdit->setVisible(true);
            mPhiMaxEdit->setVisible(true);
            break;
        }
        default:
            break;
    }
    adjustSize();
}

