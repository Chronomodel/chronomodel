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

#include "RebuildCurveDialog.h"


#include <QHBoxLayout>
#include <QPushButton>

#ifdef DEBUG
RebuildCurveDialog::RebuildCurveDialog(QStringList list, std::vector< std::pair<double, double>> *minMax, std::vector< std::pair<double, double>> *minMaxP,
                                       std::pair<double, double> *minMaxPFilter, std::pair<unsigned, unsigned> mapSize, QWidget *parent):
    QDialog{parent},
    mCompoList(list),
    mYTabMinMax(*minMax),
    mYpTabMinMax(*minMaxP)

    , mYpMinMaxFilter(*minMaxPFilter)
#else
RebuildCurveDialog::RebuildCurveDialog(QStringList list, std::vector< std::pair<double, double>> *minMax, std::vector< std::pair<double, double>> *minMaxP,
                                                  std::pair<unsigned, unsigned> mapSize, QWidget *parent):
    QDialog{parent},
    mCompoList(list),
    mYTabMinMax(*minMax),
    mYpTabMinMax(*minMaxP)

#endif
{
    setWindowTitle(tr("Rescale Density Plots"));
    QIntValidator* positiveValidator = new QIntValidator(this);
    positiveValidator->setBottom(10);
    positiveValidator->setRange(10, 5000);

    XStepEdit = new LineEdit(this);
    XStepEdit->setValidator(positiveValidator);
    XStepEdit->setPlaceholderText(tr("From 10 to 5000"));
    XStepEdit->setText(QString::number(mapSize.first));
    connect(XStepEdit, &QLineEdit::textChanged, this, &RebuildCurveDialog::setOkEnabled);

    YStepEdit = new LineEdit(this);
    YStepEdit->setValidator(positiveValidator);
    YStepEdit->setPlaceholderText(tr("From 10 to 5000"));
    YStepEdit->setText(QString::number(mapSize.second));
    connect(YStepEdit, &QLineEdit::textChanged, this, &RebuildCurveDialog::setOkEnabled);

    Y1minEdit = new QLineEdit(QLocale().toString(mYTabMinMax[0].first));
#ifdef Q_OS_WIN
    Y1minEdit->QWidget::setStyleSheet("QLineEdit { border: 0px;}");
#endif
    connect(Y1minEdit, &QLineEdit::textChanged, this, &RebuildCurveDialog::Y1MinIsValid);
    Y1maxEdit = new QLineEdit(QLocale().toString(mYTabMinMax[0].second));
#ifdef Q_OS_WIN
    Y1maxEdit->QWidget::setStyleSheet("QLineEdit { border: 0px;}");
#endif
    connect(Y1maxEdit, &QLineEdit::textChanged, this, &RebuildCurveDialog::Y1MaxIsValid);

    // Var. Rate

    Y1pminEdit = new QLineEdit(QLocale().toString(mYpTabMinMax[0].first));
#ifdef Q_OS_WIN
    Y1pminEdit->QWidget::setStyleSheet("QLineEdit { border: 0px;}");
#endif
    connect(Y1pminEdit, &QLineEdit::textChanged, this, &RebuildCurveDialog::Y1pMinIsValid);
    Y1pmaxEdit = new QLineEdit(QLocale().toString(mYpTabMinMax[0].second));
#ifdef Q_OS_WIN
    Y1pmaxEdit->QWidget::setStyleSheet("QLineEdit { border: 0px;}");
#endif
    connect(Y1pmaxEdit, &QLineEdit::textChanged, this, &RebuildCurveDialog::Y1pMaxIsValid);
#ifdef DEBUG
    if (minMaxPFilter != nullptr) {

        Y1pminFilterEdit = new QLineEdit(QLocale().toString(mYpMinMaxFilter.first));
#ifdef Q_OS_WIN
        Y1pminFilterEdit->QWidget::setStyleSheet("QLineEdit { border: 0px;}");
#endif
        connect(Y1pminFilterEdit, &QLineEdit::textChanged, this, &RebuildCurveDialog::Y1pMinFilterIsValid);
        Y1pmaxFilterEdit = new QLineEdit(QLocale().toString(mYpMinMaxFilter.second));
#ifdef Q_OS_WIN
        Y1pmaxFilterEdit->QWidget::setStyleSheet("QLineEdit { border: 0px;}");
#endif
        connect(Y1pmaxFilterEdit, &QLineEdit::textChanged, this, &RebuildCurveDialog::Y1pMaxFilterIsValid);
    }
#endif
    //buttonBox = new QDialogButtonBox(Qt::Horizontal);
    buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);

    connect(buttonBox, &QDialogButtonBox::accepted, this, &RebuildCurveDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &RebuildCurveDialog::reject);

    QBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->setSizeConstraint(QLayout::SetFixedSize);

    int ligne = 0;
    QGridLayout *midLayout = new QGridLayout;

    midLayout->addWidget(new QLabel(tr("Grid Setting")), ligne, 0, 1, 2, Qt::AlignCenter);

    ligne++;
    midLayout->addWidget(new QLabel(tr("Time Step")), ligne, 0);
    midLayout->addWidget(XStepEdit, ligne, 1);

    ligne++;
    QString str = mCompoList.join(", ");
    midLayout->addWidget(new QLabel(tr("%1 Step").arg(str)), ligne, 0);
    midLayout->addWidget(YStepEdit, ligne, 1);

    ligne++;
    _setting_1_Grid = new QGridLayout;

    QFrame* line1 = new QFrame();
    line1->setFrameShape(QFrame::HLine);
    line1->setFrameShadow(QFrame::Sunken);
    _setting_1_Grid->addWidget(line1, ligne, 0, 1, 2);
    ligne++;
    _setting_1_Grid->addWidget(new QLabel(mCompoList[0] +tr(" Axis Setting")), ligne, 0, 1, 2, Qt::AlignCenter);

    ligne++;
    str =  tr("%1 min").arg(mCompoList[0]);
    _setting_1_Grid->addWidget(new QLabel(str), ligne, 0);
    str = tr("%1 max").arg(mCompoList[0]);
    _setting_1_Grid->addWidget(new QLabel(str), ligne, 1);
    ligne++;
    _setting_1_Grid->addWidget(Y1minEdit, ligne, 0);
    _setting_1_Grid->addWidget(Y1maxEdit, ligne, 1);
    ligne++;
    str = tr("%1 Rate min").arg(mCompoList[0]);
    _setting_1_Grid->addWidget(new QLabel(str), ligne, 0);
    str = tr("%1 Rate max").arg(mCompoList[0]);
    _setting_1_Grid->addWidget(new QLabel(str), ligne, 1);
    ligne++;
    _setting_1_Grid->addWidget(Y1pminEdit, ligne, 0);
    _setting_1_Grid->addWidget(Y1pmaxEdit, ligne, 1);

    if (mCompoList.size() > 1) {

        Y2minEdit = new QLineEdit(QLocale().toString(mYTabMinMax[1].first));
#ifdef Q_OS_WIN
        Y2minEdit->QWidget::setStyleSheet("QLineEdit { border: 0px;}");
#endif
        connect(Y2minEdit, &QLineEdit::textChanged, this, &RebuildCurveDialog::Y2MinIsValid);
        Y2maxEdit = new QLineEdit(QLocale().toString(mYTabMinMax[1].second));
#ifdef Q_OS_WIN
        Y2maxEdit->QWidget::setStyleSheet("QLineEdit { border: 0px;}");
#endif
        connect(Y2maxEdit, &QLineEdit::textChanged, this, &RebuildCurveDialog::Y2MaxIsValid);

        Y2pminEdit = new QLineEdit(QLocale().toString(mYpTabMinMax[1].first));
#ifdef Q_OS_WIN
        Y2pminEdit->QWidget::setStyleSheet("QLineEdit { border: 0px;}");
#endif
        connect(Y2pminEdit, &QLineEdit::textChanged, this, &RebuildCurveDialog::Y2pMinIsValid);
        Y2pmaxEdit = new QLineEdit(QLocale().toString(mYpTabMinMax[1].second));
#ifdef Q_OS_WIN
        Y2pmaxEdit->QWidget::setStyleSheet("QLineEdit { border: 0px;}");
#endif
        connect(Y2pmaxEdit, &QLineEdit::textChanged, this, &RebuildCurveDialog::Y2pMaxIsValid);

        _setting_2_Grid = new QGridLayout;

        ligne++;
        QFrame* line2 = new QFrame();
        line2->setFrameShape(QFrame::HLine);
        line2->setFrameShadow(QFrame::Sunken);
        _setting_2_Grid->addWidget(line2, ligne, 0, 1, 2);
        ligne++;
        _setting_2_Grid->addWidget(new QLabel(tr("%1 Axis Setting").arg(mCompoList[1])), ligne, 0, 1, 2, Qt::AlignCenter);

        ligne++;
        str = tr("%1 min").arg(mCompoList[1]);
        _setting_2_Grid->addWidget(new QLabel(str), ligne, 0);
        str = tr("%1 max").arg(mCompoList[1]);
        _setting_2_Grid->addWidget(new QLabel(str), ligne, 1);
        ligne++;
        _setting_2_Grid->addWidget(Y2minEdit, ligne, 0);
        _setting_2_Grid->addWidget(Y2maxEdit, ligne, 1);
        ligne++;
        str = tr("%1 Rate min").arg(mCompoList[1]);
        _setting_2_Grid->addWidget(new QLabel(str), ligne, 0);
        str = tr("%1 Rate max").arg(mCompoList[1]);
        _setting_2_Grid->addWidget(new QLabel(str), ligne, 1);
        ligne++;
        _setting_2_Grid->addWidget(Y2pminEdit, ligne, 0);
        _setting_2_Grid->addWidget(Y2pmaxEdit, ligne, 1);

        if (mCompoList.size() > 2) {

            Y3minEdit = new QLineEdit(QLocale().toString(mYTabMinMax[2].first));
#ifdef Q_OS_WIN
            Y3minEdit->QWidget::setStyleSheet("QLineEdit { border: 0px;}");
#endif
            connect(Y3minEdit, &QLineEdit::textChanged, this, &RebuildCurveDialog::Y3MinIsValid);
            Y3maxEdit = new QLineEdit(QLocale().toString(mYTabMinMax[2].second));
#ifdef Q_OS_WIN
            Y3maxEdit->QWidget::setStyleSheet("QLineEdit { border: 0px;}");
#endif
            connect(Y3maxEdit, &QLineEdit::textChanged, this, &RebuildCurveDialog::Y3MaxIsValid);

            Y3pminEdit = new QLineEdit(QLocale().toString(mYpTabMinMax[2].first));
#ifdef Q_OS_WIN
            Y3pminEdit->QWidget::setStyleSheet("QLineEdit { border: 0px;}");
#endif
            connect(Y3pminEdit, &QLineEdit::textChanged, this, &RebuildCurveDialog::Y3pMinIsValid);
            Y3pmaxEdit = new QLineEdit(QLocale().toString(mYpTabMinMax[2].second));
#ifdef Q_OS_WIN
            Y3pmaxEdit->QWidget::setStyleSheet("QLineEdit { border: 0px;}");
#endif
            connect(Y3pmaxEdit, &QLineEdit::textChanged, this, &RebuildCurveDialog::Y3pMaxIsValid);

            _setting_3_Grid = new QGridLayout;
            ligne++;
            QFrame* line3 = new QFrame();
            line3->setFrameShape(QFrame::HLine);
            line3->setFrameShadow(QFrame::Sunken);
            _setting_3_Grid->addWidget(line3, ligne, 0, 1, 2);
            ligne++;
            _setting_3_Grid->addWidget(new QLabel(tr("%1 Axis Setting").arg(mCompoList[2])), ligne, 0, 1, 2, Qt::AlignCenter);
            ligne++;
            str = tr("%1 min").arg(mCompoList[2]);
            _setting_3_Grid->addWidget(new QLabel(str), ligne, 0);
            str = tr("%1 max").arg(mCompoList[2]);
            _setting_3_Grid->addWidget(new QLabel(str), ligne, 1);
            ligne++;
            _setting_3_Grid->addWidget(Y3minEdit, ligne, 0);
            _setting_3_Grid->addWidget(Y3maxEdit, ligne, 1);
            ligne++;
            str = tr("%1 Rate min").arg(mCompoList[2]);
            _setting_3_Grid->addWidget(new QLabel(str), ligne, 0);
            str = tr("%1 Rate max").arg(mCompoList[2]);
            _setting_3_Grid->addWidget(new QLabel(str), ligne, 1);
            ligne++;
            _setting_3_Grid->addWidget(Y3pminEdit, ligne, 0);
            _setting_3_Grid->addWidget(Y3pmaxEdit, ligne, 1);
        }

    }
#ifdef DEBUG
    else { // filter only with 1 componnent
        ligne++;
        str = tr("Filter %1 Rate min").arg(mCompoList[0]);
        _setting_1_Grid->addWidget(new QLabel(str), ligne, 0);
        str = tr("Filter %1 Rate max").arg(mCompoList[0]);
        _setting_1_Grid->addWidget(new QLabel(str), ligne, 1);
        ligne++;
        _setting_1_Grid->addWidget(Y1pminFilterEdit, ligne, 0);
        _setting_1_Grid->addWidget(Y1pmaxFilterEdit, ligne, 1);
    }
#endif
    mainLayout->addLayout(midLayout);
    mainLayout->addLayout(_setting_1_Grid);
    if (mCompoList.size() > 1) {
        mainLayout->addLayout(_setting_2_Grid);
        if (mCompoList.size() > 2)
            mainLayout->addLayout(_setting_3_Grid);
    }
    mainLayout->addWidget(buttonBox);

    setLayout(mainLayout);

    updateOptions();

}

int RebuildCurveDialog::getXSpinResult() const
{
    return XStepEdit->text().toInt();
}

int RebuildCurveDialog::getYSpinResult() const
{
    return YStepEdit->text().toInt();
}

// Y1
void RebuildCurveDialog::Y1MinIsValid(QString str)
{
    bool ok;
    QLocale locale;
    double tmp = locale.toDouble(str, &ok);
    Y1MinOK  = ok && tmp < mYTabMinMax[0].second;
    if (Y1MinOK)
        mYTabMinMax[0].first = tmp;

    setOkEnabled();
}

void RebuildCurveDialog::Y1MaxIsValid(QString str)
{
    bool ok;
    const double tmp = QLocale().toDouble(str, &ok);
    Y1MaxOK  = ok && tmp > mYTabMinMax[0].first;
    if (Y1MaxOK)
        mYTabMinMax[0].second = tmp;

    setOkEnabled();
}

// Y2
void RebuildCurveDialog::Y2MinIsValid(QString str)
{
    bool ok;
    const double tmp = QLocale().toDouble(str, &ok);
    Y2MinOK  = ok && tmp < mYTabMinMax[1].second;
    if (Y2MinOK)
        mYTabMinMax[1].first = tmp;

    setOkEnabled();
}

void RebuildCurveDialog::Y2MaxIsValid(QString str)
{
    bool ok;
    const double tmp = QLocale().toDouble(str, &ok);
    Y2MaxOK = ok && tmp > mYTabMinMax[1].first;
    if (Y2MaxOK)
        mYTabMinMax[1].second = tmp;

    setOkEnabled();
}

// Y3
void RebuildCurveDialog::Y3MinIsValid(QString str)
{
    bool ok;
    const double tmp = QLocale().toDouble(str, &ok);
    Y3MinOK  = ok && tmp < mYTabMinMax[2].second;
    if (Y3MinOK)
        mYTabMinMax[2].first = tmp;

    setOkEnabled();
}

void RebuildCurveDialog::Y3MaxIsValid(QString str)
{
    bool ok;
    const double tmp = QLocale().toDouble(str, &ok);
    Y3MaxOK = ok && tmp>mYTabMinMax[2].first;
    if (Y3MaxOK)
        mYTabMinMax[2].second = tmp;

    setOkEnabled();
}

// Y1p
void RebuildCurveDialog::Y1pMinIsValid(QString str)
{
    bool ok;
    QLocale locale;
    double tmp =locale.toDouble(str, &ok);
    Y1pMinOK  = ok && tmp < mYpTabMinMax[0].second;
    if (Y1pMinOK)
        mYpTabMinMax[0].first = tmp;

    setOkEnabled();
}

void RebuildCurveDialog::Y1pMaxIsValid(QString str)
{
    bool ok;
    const double tmp = QLocale().toDouble(str, &ok);
    Y1pMaxOK  = ok && tmp > mYpTabMinMax[0].first;
    if (Y1pMaxOK)
        mYpTabMinMax[0].second = tmp;

    setOkEnabled();
}

#ifdef DEBUG
// YP Filter
void RebuildCurveDialog::Y1pMinFilterIsValid(QString str)
{
    bool ok;
    QLocale locale;
    double tmp =locale.toDouble(str, &ok);
    Y1pMinFilterOK  = ok && tmp < mYpMinMaxFilter.second;
    if (Y1pMinFilterOK)
        mYpMinMaxFilter.first = tmp;

    setOkEnabled();
}

void RebuildCurveDialog::Y1pMaxFilterIsValid(QString str)
{
    bool ok;
    const double tmp = QLocale().toDouble(str, &ok);
    Y1pMaxFilterOK  = ok && tmp > mYpMinMaxFilter.first;
    if (Y1pMaxFilterOK)
        mYpMinMaxFilter.second = tmp;

    setOkEnabled();
}
#endif

// Y2p
void RebuildCurveDialog::Y2pMinIsValid(QString str)
{
    bool ok;
    const double tmp = QLocale().toDouble(str, &ok);
    Y2pMinOK  = ok && tmp < mYpTabMinMax[1].second;
    if (Y2pMinOK)
        mYpTabMinMax[1].first = tmp;

    setOkEnabled();
}

void RebuildCurveDialog::Y2pMaxIsValid(QString str)
{
    bool ok;
    const double tmp = QLocale().toDouble(str, &ok);
    Y2pMaxOK = ok && tmp > mYpTabMinMax[1].first;
    if (Y2pMaxOK)
        mYpTabMinMax[1].second = tmp;

    setOkEnabled();
}

// Y3p
void RebuildCurveDialog::Y3pMinIsValid(QString str)
{
    bool ok;
    const double tmp = QLocale().toDouble(str, &ok);
    Y3pMinOK  = ok && tmp < mYpTabMinMax[2].second;
    if (Y3pMinOK)
        mYpTabMinMax[2].first = tmp;

    setOkEnabled();
}

void RebuildCurveDialog::Y3pMaxIsValid(QString str)
{
    bool ok;
    const double tmp = QLocale().toDouble(str, &ok);
    Y3pMaxOK = ok && tmp>mYpTabMinMax[2].first;
    if (Y3pMaxOK)
        mYpTabMinMax[2].second = tmp;

    setOkEnabled();
}


void RebuildCurveDialog::setOkEnabled()
{
    QIntValidator* positiveValidator = new QIntValidator(this);
    positiveValidator->setBottom(10);

    bool isValid = Y1MinOK && Y1MaxOK
                   && Y2MinOK && Y2MaxOK
                   && Y3MinOK && Y3MaxOK
                   && Y1pMinOK && Y1pMaxOK
                   && Y2pMinOK && Y2pMaxOK
                   && Y3pMinOK && Y3pMaxOK
                   && XStepEdit->hasAcceptableInput()
                   && YStepEdit->hasAcceptableInput();

#ifdef DEBUG
    isValid = isValid && Y1pMinFilterOK && Y1pMaxFilterOK;
#endif

    buttonBox->button(QDialogButtonBox::Ok)->setEnabled(isValid);

}

void RebuildCurveDialog::updateOptions()
{
    YStepEdit->setEnabled(true);

    _setting_1_Grid->setEnabled(true);

    Y1minEdit->setEnabled(true);
    Y1maxEdit->setEnabled(true);
    Y1pminEdit->setEnabled(true);
    Y1pmaxEdit->setEnabled(true);

    if (mCompoList.size() > 1) {
        _setting_2_Grid->setEnabled(true);

        Y2minEdit->setEnabled(true);
        Y2maxEdit->setEnabled(true);
        Y2pminEdit->setEnabled(true);
        Y2pmaxEdit->setEnabled(true);

        if (mCompoList.size() > 2) {
            _setting_3_Grid->setEnabled(true);
            Y3minEdit->setEnabled(true);
            Y3maxEdit->setEnabled(true);
            Y3pminEdit->setEnabled(true);
            Y3pmaxEdit->setEnabled(true);
        }
    }

}

void RebuildCurveDialog::setCompoList(QStringList &list)
{
    mCompoList = list;
}

std::pair<unsigned, unsigned> RebuildCurveDialog::getMapSize() const
{
    return std::pair<unsigned, unsigned>{XStepEdit->text().toInt(), YStepEdit->text().toInt()};
}
