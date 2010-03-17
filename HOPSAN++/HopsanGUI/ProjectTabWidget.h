//!
//! @file   ProjectTabWidget.h
//! @author Björn Eriksson <bjorn.eriksson@liu.se>
//! @date   2010-02-05
//!
//! @brief Contains classes for Project Tabs
//!
//$Id$

#ifndef PROJECTTABWIDGET_H
#define PROJECTTABWIDGET_H

#include <QGraphicsView>
#include <QGraphicsScene>
#include <QTabWidget>
#include <map>

#include "MessageWidget.h"


class GUIPort;
class GUIConnector;
class ProjectTab;

class GraphicsScene : public QGraphicsScene
{
    Q_OBJECT

public:
    GraphicsScene(ProjectTab *parent = 0);
    qreal TestVar;

    ProjectTab *mpParentProjectTab;
};


class HopsanEssentials; //Forward declaration
class ComponentSystem;
class GUIComponent;

class GraphicsView : public QGraphicsView
{
    Q_OBJECT

public:
    GraphicsView(HopsanEssentials *hopsan, ComponentSystem *model, ProjectTab *parent = 0);
    ~GraphicsView();
    bool mIsCreatingConnector;
    GUIComponent *getComponent(QString name);
    GUIConnector *getTempConnector();
    ProjectTab *mpParentProjectTab;
    QMap<QString, GUIComponent *> mComponentMap;
    QMap<QString, GUIConnector *> mConnectionMap;
    ComponentSystem *getModelPointer();

signals:
    void keyPressDelete();
    void keyPressR();
    void keyPressUp();
    void keyPressDown();
    void keyPressLeft();
    void keyPressRight();
    void viewClicked();
    void checkMessages();

public slots:
    //void addComponent(QStringList parameterData, QPoint position);
    void addComponent(QString parameterType, QPoint position, QString name=QString(), bool startSelected = false);
    void deleteComponent(QString componentName);
    bool haveComponent(QString name);
    void renameComponent(QString oldName, QString newName);

    void addConnector(GUIPort *pPort);
    void removeConnector(GUIConnector* pConnector);
    void copySelected();
    void paste();

    //QByteArray *data;
    //QDataStream *stream;
    //QString *text;

protected:
    virtual void dragMoveEvent(QDragMoveEvent *event);
    virtual void dropEvent(QDropEvent *event);
    virtual void wheelEvent(QWheelEvent *event);
    virtual void mouseMoveEvent(QMouseEvent *event);
    virtual void mousePressEvent(QMouseEvent *event);
    virtual void keyPressEvent(QKeyEvent *event);
    virtual void keyReleaseEvent(QKeyEvent *event);

    void contextMenuEvent ( QContextMenuEvent * event );

private:
    GUIConnector *mpTempConnector;
    HopsanEssentials *mpHopsan;
    ComponentSystem *mpModel;
    QStringList mCopyData;
    QList<QPointF> mCopyDataPos;
    //! @todo QMap no good means problem if we rename need to loop around the rename like in coore
 };


class ProjectTabWidget; //Forward declaration

class ProjectTab : public QWidget
{
    Q_OBJECT

public:
    ProjectTab(ProjectTabWidget *parent = 0);
    bool mIsSaved;
    QString mModelFileName;
    ProjectTabWidget *mpParentProjectTabWidget;
    ComponentSystem *mpComponentSystem;

    GraphicsView *mpGraphicsView;
    GraphicsScene *mpGraphicsScene;

public slots:
    void hasChanged();

signals:
    void checkMessages();

};


class MainWindow;

class ProjectTabWidget : public QTabWidget
{
    Q_OBJECT

public:
    ProjectTabWidget(MainWindow *parent = 0);

    ProjectTab *getCurrentTab();

    MainWindow *mpParentMainWindow;

    HopsanEssentials *mpHopsan;

    size_t mNumberOfUntitledTabs;

public slots:
    void addProjectTab(ProjectTab *projectTab, QString tabName="Untitled");
    void addNewProjectTab(QString tabName="Untitled");
    void saveProjectTab();
    void saveProjectTabAs();
    void saveProjectTab(int index, bool saveAs);
    bool closeProjectTab(int index);
    bool closeAllProjectTabs();
    void simulateCurrent();
    void loadModel();
    void saveModel(bool saveAs);

signals:
    void checkMessages();

};

#endif // PROJECTTABWIDGET_H
