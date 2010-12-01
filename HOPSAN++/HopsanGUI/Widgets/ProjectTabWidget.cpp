//!
//! @file   ProjectTabWidget.cpp
//! @author Björn Eriksson <bjorn.eriksson@liu.se>
//! @date   2010-02-05
//!
//! @brief Contain classes for Project Tabs
//!
//$Id$

#include <QtGui>
#include <QSizePolicy>
#include <QHash>
#include <QtXml>

#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <cassert>

#include "ProjectTabWidget.h"
#include "../MainWindow.h"
#include "../GUIPort.h"
#include "MessageWidget.h"
#include "../InitializationThread.h"
#include "../SimulationThread.h"
#include "../ProgressBarThread.h"
#include "../UndoStack.h"
#include "LibraryWidget.h"
#include "../GUIObjects/GUIObject.h"
#include "../GUIConnector.h"
#include "../GraphicsView.h"
#include "../GUIObjects/GUISystem.h"
#include "PlotWidget.h"
#include "../Configuration.h"
#include "../version.h"
#include "../Utilities/GUIUtilities.h"
#include "../loadObjects.h"


//! @class ProjectTab
//! @brief The ProjectTab class is a Widget to contain a simulation model
//!
//! ProjectTab contains a drawing space to create models.
//!


//! Constructor.
//! @param parent defines a parent to the new instanced object.
ProjectTab::ProjectTab(ProjectTabWidget *parent)
    : QWidget(parent)
{
    mpParentProjectTabWidget = parent;
    mpSystem = new GUISystem(this, 0);

    connect(this, SIGNAL(checkMessages()), gpMainWindow->mpMessageWidget, SLOT(checkMessages()));

    emit checkMessages();

    double timeStep = mpSystem->getCoreSystemAccessPtr()->getDesiredTimeStep();

    gpMainWindow->setTimeStepInToolBar(timeStep);

    mIsSaved = true;

    mpGraphicsView  = new GraphicsView(this);
    mpGraphicsView->setScene(mpSystem->getContainedScenePtr());

    QVBoxLayout *tabLayout = new QVBoxLayout;
    tabLayout->addWidget(mpGraphicsView);
    this->setLayout(tabLayout);
}

ProjectTab::~ProjectTab()
{
    //! @todo do we need to call inheritet class destructor also
    //qDebug() << "projectTab destructor";

    for(int i=0; i<mpSystem->getPortListPtrs().size(); ++i)
    {
        disconnect(gpMainWindow->hidePortsAction,SIGNAL(triggered(bool)),mpSystem->getPortListPtrs().at(i), SLOT(hideIfNotConnected(bool)));
        disconnect(mpGraphicsView, SIGNAL(zoomChange(qreal)), mpSystem->getPortListPtrs().at(i), SLOT(setPortOverlayScale(qreal)));
    }

    delete mpSystem;
    //! @todo do we need to delete the graphicsiew or is that handled automatically
}


//! Should be called when a model has changed in some sense,
//! e.g. a component added or a connection has changed.
void ProjectTab::hasChanged()
{
    if (mIsSaved)
    {
        QString tabName = mpParentProjectTabWidget->tabText(mpParentProjectTabWidget->currentIndex());

        if(!tabName.endsWith("*"))
        {
            tabName.append("*");
        }
        mpParentProjectTabWidget->setTabText(mpParentProjectTabWidget->currentIndex(), tabName);

        mIsSaved = false;
    }
}


//! Returns whether or not the current project is saved
bool ProjectTab::isSaved()
{
    return mIsSaved;
}


//! Set function to tell the tab whether or not it is saved
void ProjectTab::setSaved(bool value)
{
    mIsSaved = value;
    QString tabName = mpParentProjectTabWidget->tabText(mpParentProjectTabWidget->currentIndex());
    while(tabName.endsWith("*"))
    {
        tabName.chop(1);
        mpParentProjectTabWidget->setTabText(mpParentProjectTabWidget->currentIndex(), tabName);
    }
}


//! Simulates the model in the tab in a separate thread, the GUI runs a progressbar parallel to the simulation.
bool ProjectTab::simulate()
{

    MessageWidget *pMessageWidget = gpMainWindow->mpMessageWidget;

    mpSystem->updateStartTime();
    mpSystem->updateStopTime();
    mpSystem->updateTimeStep();

        //Setup simulation parameters
    double startTime = mpSystem->getStartTime();
    double finishTime = mpSystem->getStopTime();
    double dt = finishTime - startTime;
    size_t nSteps = dt/mpSystem->getCoreSystemAccessPtr()->getDesiredTimeStep();
    size_t nSamples = mpSystem->getNumberOfLogSamples();

    if(!mpSystem->getCoreSystemAccessPtr()->isSimulationOk())
    {
        emit checkMessages();
        return false;
    }

    qDebug() << "Initializing simulation: " << startTime << nSteps << finishTime;

        //Ask core to initialize simulation
    InitializationThread actualInitialization(mpSystem->getCoreSystemAccessPtr(), startTime, finishTime, nSamples, this);
    actualInitialization.start();
    actualInitialization.setPriority(QThread::HighestPriority);

    ProgressBarThread progressThread(this);
    QProgressDialog progressBar(tr("Initializing simulation..."), tr("&Abort initialization"), 0, 0, this);
    if(gConfig.getEnableProgressBar())
    {
        progressBar.setWindowModality(Qt::WindowModal);
        progressBar.setWindowTitle(tr("Simulate!"));
        size_t i=0;
        while (actualInitialization.isRunning())
        {
            progressThread.start();
            progressThread.setPriority(QThread::TimeCriticalPriority);//(QThread::LowestPriority);
            progressThread.wait();
            progressBar.setValue(i++);
            if (progressBar.wasCanceled())
            {
                mpSystem->getCoreSystemAccessPtr()->stop();
            }
        }
        progressBar.setValue(i);
    }

    actualInitialization.wait(); //Make sure actualSimulation do not goes out of scope during simulation
    actualInitialization.quit();


        //Ask core to execute (and finalize) simulation
    QTime simTimer;
    if (!progressBar.wasCanceled())
    {
        if(gConfig.getUseMulticore())
            gpMainWindow->mpMessageWidget->printGUIInfoMessage("Starting Multi Threaded Simulation");
        else
            gpMainWindow->mpMessageWidget->printGUIInfoMessage("Starting Single Threaded Simulation");


        simTimer.start();
        SimulationThread actualSimulation(mpSystem->getCoreSystemAccessPtr(), startTime, finishTime, this);
        actualSimulation.start();
        actualSimulation.setPriority(QThread::HighestPriority);
            //! @todo TimeCriticalPriority seem to work on dual core, is it a problem on single core machines only?
        //actualSimulation.setPriority(QThread::TimeCriticalPriority); //No bar appears in Windows with this prio

        if(gConfig.getEnableProgressBar())
        {
            progressBar.setLabelText(tr("Running simulation..."));
            progressBar.setCancelButtonText(tr("&Abort simulation"));
            progressBar.setMinimum(0);
            progressBar.setMaximum(nSteps);
            while (actualSimulation.isRunning())
            {
               progressThread.start();
               progressThread.setPriority(QThread::LowestPriority);
               progressThread.wait();
               progressBar.setValue((size_t)(mpSystem->getCoreSystemAccessPtr()->getCurrentTime()/dt * nSteps));
               if (progressBar.wasCanceled())
               {
                  mpSystem->getCoreSystemAccessPtr()->stop();
               }
            }
            progressThread.quit();
            progressBar.setValue((size_t)(mpSystem->getCoreSystemAccessPtr()->getCurrentTime()/dt * nSteps));
        }

        actualSimulation.wait(); //Make sure actualSimulation do not goes out of scope during simulation
        actualSimulation.quit();
        emit checkMessages();
    }

    QString timeString;
    timeString.setNum(simTimer.elapsed());
    if (progressBar.wasCanceled())
    {
        pMessageWidget->printGUIInfoMessage(QString(tr("Simulation of '").append(mpSystem->getCoreSystemAccessPtr()->getRootSystemName()).append(tr("' was terminated!"))));
    }
    else
    {
        pMessageWidget->printGUIInfoMessage(QString(tr("Simulated '").append(mpSystem->getCoreSystemAccessPtr()->getRootSystemName()).append(tr("' successfully!  Simulation time: ").append(timeString).append(" ms"))));
        emit simulationFinished();
        //this->mpParentProjectTabWidget->mpParentMainWindow->mpPlotWidget->mpVariableList->updateList();
    }
    emit checkMessages();

    return !(progressBar.wasCanceled());
}


//! Slot that saves current project to old file name if it exists.
//! @see saveProjectTab(int index)
void ProjectTab::save()
{
    saveModel(EXISTINGFILE);
}


//! Slot that saves current project to a new model file.
//! @see saveProjectTab(int index)
void ProjectTab::saveAs()
{
    saveModel(NEWFILE);
}


//! Saves the model and the viewport settings in the tab to a model file.
//! @param saveAsFlag tells whether or not an already existing file name shall be used
//! @see saveProjectTab()
//! @see loadModel()
void ProjectTab::saveModel(saveTarget saveAsFlag)
{
        //Remove the asterix if tab goes from unsaved to saved
    if(!mIsSaved)
    {
        QString tabName = mpParentProjectTabWidget->tabText(mpParentProjectTabWidget->currentIndex());
        tabName.chop(1);
        mpParentProjectTabWidget->setTabText(mpParentProjectTabWidget->currentIndex(), tabName);
        std::cout << "ProjectTabWidget: " << qPrintable(QString("Project: ").append(tabName).append(QString(" saved"))) << std::endl;
        this->setSaved(true);
    }

    if((mpSystem->mModelFileInfo.filePath().isEmpty()) || (saveAsFlag == NEWFILE))
    {
        QDir fileDialogSaveDir;
        QString modelFilePath;
        modelFilePath = QFileDialog::getSaveFileName(this, tr("Save Model File"),
                                                             fileDialogSaveDir.currentPath() + QString(MODELPATH),
                                                             tr("Hopsan Model Files (*.hmf)"));
        mpSystem->mModelFileInfo.setFile(modelFilePath);
    }

    //! @obsolete quickhack to avoid saving hmfx over hmf
    //! @obsolete we shall allways save xml format as hmf from now on
    if (mpSystem->mModelFileInfo.filePath().endsWith("x"))
    {
        QString tmp = mpSystem->mModelFileInfo.filePath();
        tmp.chop(1);
        mpSystem->mModelFileInfo.setFile(tmp);
    }

    QFile file(mpSystem->mModelFileInfo.filePath());   //Create a QFile object
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))  //open file
    {
        qDebug() << "Failed to open file for writing: " + mpSystem->mModelFileInfo.filePath();
        return;
    }

    //Sets the model name (must set this name before saving or else systemports wont know the real name of their rootsystem parent)
    mpSystem->setName(mpSystem->mModelFileInfo.baseName());

    //Save xml document
    QDomDocument domDocument;
    QDomElement hmfRoot = appendHMFRootElement(domDocument);

    //Save the model component hierarcy
    //! @todo maybe use a saveload object instead of calling save imediately (only load object exist for now), or maybe this is fine
    mpSystem->saveToDomElement(hmfRoot);

    //Save to file
    const int IndentSize = 4;
    QFile xmlhmf(mpSystem->mModelFileInfo.filePath());
    if (!xmlhmf.open(QIODevice::WriteOnly | QIODevice::Text))  //open file
    {
        qDebug() << "Failed to open file for writing: " << mpSystem->mModelFileInfo.filePath() << "x";
        return;
    }
    QTextStream out(&xmlhmf);
    appendRootXMLProcessingInstruction(domDocument); //The xml "comment" on the first line
    domDocument.save(out, IndentSize);
}



//! @class ProjectTabWidget
//! @brief The ProjectTabWidget class is a container class for ProjectTab class
//!
//! ProjectTabWidget contains ProjectTabWidget widgets.
//!


//! Constructor.
//! @param parent defines a parent to the new instanced object.
ProjectTabWidget::ProjectTabWidget(MainWindow *parent)
        :   QTabWidget(parent)
{
    connect(this, SIGNAL(checkMessages()), gpMainWindow->mpMessageWidget, SLOT(checkMessages()));

    setTabsClosable(true);
    mNumberOfUntitledTabs = 0;

//    mpCopyData = new QString;

    connect(this,SIGNAL(currentChanged(int)),SLOT(tabChanged()));
    connect(this,SIGNAL(tabCloseRequested(int)),SLOT(closeProjectTab(int)));
    connect(this,SIGNAL(tabCloseRequested(int)),SLOT(tabChanged()));

    connect(gpMainWindow->newAction, SIGNAL(triggered()), this, SLOT(addNewProjectTab()));
    connect(gpMainWindow->openAction, SIGNAL(triggered()), this, SLOT(loadModel()));
}


//!  Tells current tab to export itself to PDF. This is needed because a direct connection to current tab would be too complicated.


//! Returns a pointer to the currently active project tab - be sure to check that the number of tabs is not zero before calling this
ProjectTab *ProjectTabWidget::getCurrentTab()
{
    return qobject_cast<ProjectTab *>(currentWidget());
}


//! Returns a pointer to the currently active project tab - be sure to check that the number of tabs is not zero before calling this
ProjectTab *ProjectTabWidget::getTab(int index)
{
    return qobject_cast<ProjectTab *>(widget(index));
}


//! Returns a pointer to the currently system model - be sure to check that the number of tabs is not zero before calling this
GUISystem *ProjectTabWidget::getCurrentSystem()
{
    return getCurrentTab()->mpSystem;
}


//! Returns a pointer to the currently system model - be sure to check that the number of tabs is not zero before calling this
GUISystem *ProjectTabWidget::getSystem(int index)
{
    return getTab(index)->mpSystem;
}


//! Adds an existing ProjectTab object to itself.
//! @see closeProjectTab(int index)
void ProjectTabWidget::addProjectTab(ProjectTab *projectTab, QString tabName)
{
    projectTab->setParent(this);

    addTab(projectTab, tabName);
    setCurrentWidget(projectTab);

    emit newTabAdded();
}


//! Adds a ProjectTab object (a new tab) to itself.
//! @see closeProjectTab(int index)
void ProjectTabWidget::addNewProjectTab(QString tabName)
{
    tabName.append(QString::number(mNumberOfUntitledTabs));

    ProjectTab *newTab = new ProjectTab(this);
    newTab->mpSystem->setName(tabName);

    this->addTab(newTab, tabName);
    this->setCurrentWidget(newTab);

    mNumberOfUntitledTabs += 1;
}


//! Closes current project.
//! @param index defines which project to close.
//! @return true if closing went ok. false if the user canceled the operation.
//! @see closeAllProjectTabs()
bool ProjectTabWidget::closeProjectTab(int index)
{
    if (!(this->getTab(index)->isSaved()))
    {
        QString modelName;
        modelName = tabText(index);
        modelName.chop(1);
        QMessageBox msgBox;
        msgBox.setText(QString("The model '").append(modelName).append("'").append(QString(" is not saved.")));
        msgBox.setInformativeText("Do you want to save your changes before closing?");
        msgBox.setStandardButtons(QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
        msgBox.setDefaultButton(QMessageBox::Save);

        int answer = msgBox.exec();

        switch (answer)
        {
        case QMessageBox::Save:
            // Save was clicked
            getTab(index)->save();
            break;
        case QMessageBox::Discard:
            // Don't Save was clicked
            break;
        case QMessageBox::Cancel:
            // Cancel was clicked
            return false;
        default:
            // should never be reached
            return false;
        }
    }

    //Disconnect signals
    //std::cout << "ProjectTabWidget: " << "Closing project: " << qPrintable(tabText(index)) << std::endl;
    //statusBar->showMessage(QString("Closing project: ").append(tabText(index)));
    disconnect(gpMainWindow->resetZoomAction,       SIGNAL(triggered()),    getTab(index)->mpGraphicsView,  SLOT(resetZoom()));
    disconnect(gpMainWindow->zoomInAction,          SIGNAL(triggered()),    getTab(index)->mpGraphicsView,  SLOT(zoomIn()));
    disconnect(gpMainWindow->zoomOutAction,         SIGNAL(triggered()),    getTab(index)->mpGraphicsView,  SLOT(zoomOut()));
    disconnect(gpMainWindow->exportPDFAction,       SIGNAL(triggered()),    getTab(index)->mpGraphicsView,  SLOT(exportToPDF()));
    disconnect(gpMainWindow->centerViewAction,      SIGNAL(triggered()),    getTab(index)->mpGraphicsView,  SLOT(centerView()));
    disconnect(gpMainWindow->hideNamesAction,       SIGNAL(triggered()),    getSystem(index),               SLOT(hideNames()));
    disconnect(gpMainWindow->showNamesAction,       SIGNAL(triggered()),    getSystem(index),               SLOT(showNames()));
    disconnect(gpMainWindow->mpStartTimeLineEdit,   SIGNAL(editingFinished()), getSystem(index),            SLOT(updateStartTime()));
    disconnect(gpMainWindow->mpTimeStepLineEdit,    SIGNAL(editingFinished()), getSystem(index),            SLOT(updateTimeStep()));
    disconnect(gpMainWindow->mpFinishTimeLineEdit,  SIGNAL(editingFinished()), getSystem(index),            SLOT(updateStopTime()));
    disconnect(gpMainWindow->disableUndoAction,     SIGNAL(triggered()),    getSystem(index),               SLOT(disableUndo()));
    disconnect(gpMainWindow->simulateAction,        SIGNAL(triggered()),    getTab(index),                  SLOT(simulate()));
    disconnect(gpMainWindow->mpStartTimeLineEdit,   SIGNAL(editingFinished()), getSystem(index),            SLOT(updateStartTime()));
    disconnect(gpMainWindow->mpFinishTimeLineEdit,  SIGNAL(editingFinished()), getSystem(index),            SLOT(updateStopTime()));
    disconnect(gpMainWindow->mpTimeStepLineEdit,    SIGNAL(editingFinished()), getSystem(index),            SLOT(updateTimeStep()));
    disconnect(gpMainWindow->saveAction,            SIGNAL(triggered()),    getTab(index),                  SLOT(save()));
    disconnect(gpMainWindow->saveAsAction,          SIGNAL(triggered()),    getTab(index),                  SLOT(saveAs()));

    //Delete project tab
    delete widget(index);
    //We dont need to call removeTab here, this seems to be handled automatically
    return true;
}


//! Closes all opened projects.
//! @return true if closing went ok. false if the user canceled the operation.
//! @see closeProjectTab(int index)
//! @see saveProjectTab()
bool ProjectTabWidget::closeAllProjectTabs()
{
    gConfig.clearLastSessionModels();

    while(count() > 0)
    {
        setCurrentIndex(count()-1);
        gConfig.addLastSessionModel(getCurrentSystem()->mModelFileInfo.filePath());
        if (!closeProjectTab(count()-1))
        {
            return false;
        }
    }
    return true;
}


//! Loads a model from a file and opens it in a new project tab.
//! @see loadModel(QString modelFileName)
//! @see Model(saveTarget saveAsFlag)
void ProjectTabWidget::loadModel()
{
    QDir fileDialogOpenDir;
    QString modelFileName = QFileDialog::getOpenFileName(this, tr("Choose Model File"),
                                                         fileDialogOpenDir.currentPath() + QString(MODELPATH),
                                                         tr("Hopsan Model Files (*.hmf *.hmfx)"));
    loadModel(modelFileName);

    emit newTabAdded();
}


//! Loads a model from a file and opens it in a new project tab.
//! @param modelFileName is the path to the loaded file
//! @see loadModel()
//! @see saveModel(saveTarget saveAsFlag)
void ProjectTabWidget::loadModel(QString modelFileName)
{
    //! @todo maybe  write utility function that opens filel checks existance and sets fileinfo
    QFile file(modelFileName);   //Create a QFile object
    if(!file.exists())
    {
        qDebug() << "File not found: " + file.fileName();
        return;
    }
    QFileInfo fileInfo(file);

    //Make sure file not already open
    for(int t=0; t!=this->count(); ++t)
    {
        if( (this->tabText(t) == fileInfo.fileName()) || (this->tabText(t) == (fileInfo.fileName() + "*")) )
        {
            QMessageBox::StandardButton reply;
            reply = QMessageBox::information(this, tr("Error"), tr("Unable to load model. File is already open."));
            return;
        }
    }

    gpMainWindow->registerRecentModel(fileInfo);

    this->addProjectTab(new ProjectTab(this), fileInfo.fileName());
    ProjectTab *pCurrentTab = this->getCurrentTab();
    pCurrentTab->setSaved(true);

    //Check if this is an expected hmf xml file
    //! @todo maybe write helpfunction that does this directly in system (or container)
    QDomDocument domDocument;
    QDomElement hmfRoot = loadXMLDomDocument(file, domDocument, HMF_ROOTTAG);
    if (!hmfRoot.isNull())
    {
        //! @todo Check version numbers
        //! @todo check if we could load else give error message and dont attempt to load
        QDomElement systemElement = hmfRoot.firstChildElement(HMF_SYSTEMTAG);
        pCurrentTab->mpSystem->setModelFileInfo(file); //Remember info about the file from which the data was loaded
        pCurrentTab->mpSystem->loadFromDomElement(systemElement);
    }
    else
    {
        //! @obsolete
        //! Should be removed later
        //Assume that this is an old text baseed hmf file load it and convert it
        pCurrentTab->mpSystem->loadFromHMF(modelFileName);
        pCurrentTab->save();
    }
}


void ProjectTabWidget::tabChanged()
{
    for(int i=0; i<count(); ++i)
    {
            //If you add a disconnect here, remember to also add it to the close tab function!
        disconnect(gpMainWindow->resetZoomAction,   SIGNAL(triggered()),    getTab(i)->mpGraphicsView,  SLOT(resetZoom()));
        disconnect(gpMainWindow->zoomInAction,      SIGNAL(triggered()),    getTab(i)->mpGraphicsView,  SLOT(zoomIn()));
        disconnect(gpMainWindow->zoomOutAction,     SIGNAL(triggered()),    getTab(i)->mpGraphicsView,  SLOT(zoomOut()));
        disconnect(gpMainWindow->exportPDFAction,   SIGNAL(triggered()),    getTab(i)->mpGraphicsView,  SLOT(exportToPDF()));
        disconnect(gpMainWindow->centerViewAction,  SIGNAL(triggered()),    getTab(i)->mpGraphicsView,  SLOT(centerView()));
        disconnect(gpMainWindow->hideNamesAction,   SIGNAL(triggered()),    getSystem(i),               SLOT(hideNames()));
        disconnect(gpMainWindow->showNamesAction,   SIGNAL(triggered()),    getSystem(i),               SLOT(showNames()));
        disconnect(gpMainWindow->mpStartTimeLineEdit,   SIGNAL(editingFinished()),  getSystem(i),       SLOT(updateStartTime()));
        disconnect(gpMainWindow->mpTimeStepLineEdit,    SIGNAL(editingFinished()),  getSystem(i),       SLOT(updateTimeStep()));
        disconnect(gpMainWindow->mpFinishTimeLineEdit,  SIGNAL(editingFinished()),  getSystem(i),       SLOT(updateStopTime()));
        disconnect(gpMainWindow->disableUndoAction,     SIGNAL(triggered()),        getSystem(i),       SLOT(disableUndo()));
        disconnect(gpMainWindow->simulateAction,        SIGNAL(triggered()),        getTab(i),          SLOT(simulate()));
        disconnect(gpMainWindow->mpStartTimeLineEdit,   SIGNAL(editingFinished()),  getSystem(i),       SLOT(updateStartTime()));
        disconnect(gpMainWindow->mpFinishTimeLineEdit,  SIGNAL(editingFinished()),  getSystem(i),       SLOT(updateStopTime()));
        disconnect(gpMainWindow->mpTimeStepLineEdit,    SIGNAL(editingFinished()),  getSystem(i),       SLOT(updateTimeStep()));
        disconnect(gpMainWindow->saveAction,            SIGNAL(triggered()),        getTab(i),          SLOT(save()));
        disconnect(gpMainWindow->saveAsAction,          SIGNAL(triggered()),        getTab(i),          SLOT(saveAs()));
        disconnect(gpMainWindow->cutAction,             SIGNAL(triggered()),        getSystem(i),       SLOT(cutSelected()));
        disconnect(gpMainWindow->copyAction,            SIGNAL(triggered()),        getSystem(i),       SLOT(copySelected()));
        disconnect(gpMainWindow->pasteAction,           SIGNAL(triggered()),        getSystem(i),       SLOT(paste()));
        disconnect(gpMainWindow->propertiesAction,     SIGNAL(triggered()),        getSystem(i),       SLOT(openPropertiesDialogSlot()));
    }
    if(this->count() != 0)
    {
        connect(gpMainWindow->resetZoomAction,      SIGNAL(triggered()),        getCurrentTab()->mpGraphicsView,    SLOT(resetZoom()));
        connect(gpMainWindow->zoomInAction,         SIGNAL(triggered()),        getCurrentTab()->mpGraphicsView,    SLOT(zoomIn()));
        connect(gpMainWindow->zoomOutAction,        SIGNAL(triggered()),        getCurrentTab()->mpGraphicsView,    SLOT(zoomOut()));
        connect(gpMainWindow->exportPDFAction,      SIGNAL(triggered()),        getCurrentTab()->mpGraphicsView,    SLOT(exportToPDF()));
        connect(gpMainWindow->centerViewAction,     SIGNAL(triggered()),        getCurrentTab()->mpGraphicsView,    SLOT(centerView()));
        connect(gpMainWindow->hideNamesAction,      SIGNAL(triggered()),        getCurrentSystem(),     SLOT(hideNames()));
        connect(gpMainWindow->showNamesAction,      SIGNAL(triggered()),        getCurrentSystem(),     SLOT(showNames()));
        connect(gpMainWindow->mpStartTimeLineEdit,  SIGNAL(editingFinished()),  getCurrentSystem(),     SLOT(updateStartTime()));
        connect(gpMainWindow->mpTimeStepLineEdit,   SIGNAL(editingFinished()),  getCurrentSystem(),     SLOT(updateTimeStep()));
        connect(gpMainWindow->mpFinishTimeLineEdit, SIGNAL(editingFinished()),  getCurrentSystem(),     SLOT(updateStopTime()));
        connect(gpMainWindow->disableUndoAction,    SIGNAL(triggered()),        getCurrentSystem(),     SLOT(disableUndo()));
        connect(gpMainWindow->simulateAction,       SIGNAL(triggered()),        getCurrentTab(),        SLOT(simulate()));
        connect(gpMainWindow->mpStartTimeLineEdit,  SIGNAL(editingFinished()),  getCurrentSystem(),     SLOT(updateStartTime()));
        connect(gpMainWindow->mpFinishTimeLineEdit, SIGNAL(editingFinished()),  getCurrentSystem(),     SLOT(updateStopTime()));
        connect(gpMainWindow->mpTimeStepLineEdit,   SIGNAL(editingFinished()),  getCurrentSystem(),     SLOT(updateTimeStep()));
        connect(gpMainWindow->saveAction,           SIGNAL(triggered()),        getCurrentTab(),        SLOT(save()));
        connect(gpMainWindow->saveAsAction,         SIGNAL(triggered()),        getCurrentTab(),        SLOT(saveAs()));
        connect(gpMainWindow->cutAction,            SIGNAL(triggered()),        getCurrentSystem(),     SLOT(cutSelected()));
        connect(gpMainWindow->copyAction,           SIGNAL(triggered()),        getCurrentSystem(),     SLOT(copySelected()));
        connect(gpMainWindow->pasteAction,          SIGNAL(triggered()),        getCurrentSystem(),     SLOT(paste()));
        connect(gpMainWindow->propertiesAction,    SIGNAL(triggered()),        getCurrentSystem(),     SLOT(openPropertiesDialogSlot()));
        getCurrentSystem()->updateUndoStatus();
        getCurrentSystem()->updateSimulationParametersInToolBar();
    }
}
