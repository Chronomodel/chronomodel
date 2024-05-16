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

#include "ProjectView_bash.h"

#include <QDir>
#include <QFileDialog>

ProjectView::ProjectView(std::shared_ptr<Project>&, QWidget* parent, Qt::WindowFlags flags):QWidget(parent, flags)
{
    mTable = new QTableWidget(this);
    mTable->setColumnCount(1);
    mTable->setHorizontalHeaderLabels({"Project Files"});
    mTable->setTextElideMode(Qt::ElideNone);

    mAddButton = new QPushButton ("Add File", this);
    mRemoveButton = new QPushButton ("Remove File", this);

    connect(mAddButton, &QPushButton::pressed, this, &ProjectView::tableAdd);
    connect(mRemoveButton, &QPushButton::pressed, this, &ProjectView::tableRemove);

    mTable->setFixedWidth(width()/2);
    mTable->setGeometry(QRect(width()/4, 50, width()/2, height()));

    mAddButton->setGeometry(QRect(0, 0, 150, 0));
    mRemoveButton->setGeometry(QRect(10, mAddButton->pos().y() + mAddButton->height() + 10, 150, 0));

    mLog = new QTextEdit(this);
    setMinimumSize(500, 200);
}

ProjectView::~ProjectView()
{

}

void ProjectView::setScreenDefinition()
{

}

void ProjectView::resizeEvent(QResizeEvent*)
{
    mTable->setFixedWidth(width()/3);
    mTable->setColumnWidth(0, width()/3);
    mTable->setGeometry(QRect(width()/3, 20, width()/3, height() - 20));

    auto x1 = mTable->pos().x();
    x1 = x1 /2 - 75;
    mAddButton->setGeometry(QRect(x1, 50, 150, 50));

    mRemoveButton->setGeometry(QRect(x1, mAddButton->pos().y() + mAddButton->height() + 10, 150, 50));

    x1 = mTable->pos().x() + mTable->width();

    mLog->setGeometry(QRect(x1 + 10, mTable->pos().y(), width() - x1 - 20, mTable->height()));

}

void ProjectView::tableAdd()
{
    const QString currentPath = QDir::homePath();
    const QString path = QFileDialog::getOpenFileName(this,
                                                      tr("Open File"),
                                                      currentPath,
                                                      tr("Chronomodel Project (*.chr)"));

    if (!path.isEmpty()) {
        mTable->setRowCount(mTable->rowCount()+1);

        QTableWidgetItem *newItem = new QTableWidgetItem(path);
        mTable->setItem(mTable->rowCount()-1 , 0, newItem );
        mTable->item(mTable->rowCount()-1 , 0)->setToolTip(path);
    }
}

void ProjectView::tableRemove()
{
    for (const auto i : mTable->selectedItems())
        mTable->removeRow(i->row());
}

void ProjectView::applyFilesSettings(Model*)
{

}

