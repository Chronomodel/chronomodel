/* ---------------------------------------------------------------------

Copyright or © or Copr. CNRS	2014 - 2018

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

#include "AboutDialog.h"
#include "Painting.h"
#include <QtWidgets>


AboutDialog::AboutDialog(QWidget* parent, Qt::WindowFlags flags):QDialog(parent, flags)
{
    setWindowTitle(tr("About Chronomodel"));

    // -----------

    mLabel = new QLabel();
    mLabel->setTextFormat(Qt::RichText);
    mLabel->setOpenExternalLinks(true);
    mLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
    mLabel->setWordWrap(true);

    QString text = "<b>" + qApp->applicationName() + " " + qApp->applicationVersion() + "</b><br><br>";
    text += "<a href=\"http://www.chronomodel.com\">http://www.chronomodel.com</a><br><br>";
    text += "Copyright © CNRS 2014 - 2018<br> ";

    text += "<b>Project director</b> : Philippe LANOS<br>";

    text += "<b>Contributors</b> : Helori LANOS, Philippe DUFRESNE<br><br>";

    text += "<b>Contact</b> :<br>";
    text += "<a href=\"mailto:support@chronomodel.com \">support@chronomodel.com </a><br><br>";

    text += "<b>License</b> :<br>";
    text += "Chronomodel is released under the <a href=\"http://www.cecill.info/licences/Licence_CeCILL_V2.1-en.html\">CeCILL License V2.1</a><br><br>";

    mLabel->setText(text);

    QVBoxLayout* layout = new QVBoxLayout();
    layout->addWidget(mLabel);
    setLayout(layout);

    setMinimumWidth(600);
}

AboutDialog::~AboutDialog()
{

}

void AboutDialog::paintEvent(QPaintEvent* e)
{
    Q_UNUSED(e);
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    QPixmap icon(":chronomodel.png");
    int s = 200;
    int m = 50;
    p.drawPixmap(QRect(width() - m - s, m, s, s), icon, icon.rect());

}
