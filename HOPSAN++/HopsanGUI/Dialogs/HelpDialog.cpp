//!
//! @file   HelpDialog.cpp
//! @author Robert Braun <robert.braun@liu.se>
//! @date   2010-XX-XX
//!
//! @brief Contains a class for the Help dialog
//!
//$Id: HelpDialog.cpp 2426 2010-12-30 19:58:15Z petno25 $

#include "../MainWindow.h"
#include "HelpDialog.h"
#include <QWebView>
#include <QGridLayout>
#include <QPushButton>

#include "../common.h"
#include "../version.h"


//! @class HelpDialog
//! @brief A class for displaying the "Help" dialog
//!
//! Shows the HTML user guide from Doxygen
//!

//! Constructor for the help dialog
//! @param parent Pointer to the main window
HelpDialog::HelpDialog(MainWindow *parent)
    : QDialog(parent)
{
    this->setObjectName("HelpDialog");
    this->resize(480,640);
    this->setWindowTitle("Hopsan NG User Guide");

    mpHelp = new QWebView(this);
    qDebug() << gExecPath << QString(HELPPATH) << "hopsan-user.html";
    mpHelp->load(QUrl::fromLocalFile(gExecPath + QString(HELPPATH) + "hopsan-user.html"));

    mpOkButton = new QPushButton(tr("&Close"));
    mpOkButton->setAutoDefault(true);
    mpOkButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    connect(mpOkButton, SIGNAL(clicked()), this, SLOT(close()));

    mpLayout = new QGridLayout;
    mpLayout->setSizeConstraint(QLayout::SetFixedSize);
    mpLayout->addWidget(mpHelp, 0, 0);
    setLayout(mpLayout);
}
