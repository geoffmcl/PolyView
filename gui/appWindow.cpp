// MIT License Terms (http://en.wikipedia.org/wiki/MIT_License)
// 
// Copyright (C) 2011 by Oleg Alexandrov
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
#include <qapplication.h>
#ifdef USE_QT4_DEFS // Q3PopupWindow, and ... subs
#include <Q3PopupMenu>
#include <Q3MainWindow>
//#include <QtGui/Q3MainWindow>
#else
#include <QMenu>    // was <Q3PopupMenu>
//#include <QMainWindow> // was <Q3MainWindow>
#include <qmainwindow.h>
#endif
#include <qlabel.h>
#include <qmenubar.h>
#include <qmessagebox.h>
#include <qstatusbar.h>
#include <qlayout.h>
#include <QMenu>
#include <QEvent>
#ifdef ADD_PREF_INI
#include <QSettings>
#endif // ADD_PREF_INI
//#ifdef _MSC_VER // drop Q3TextEdit, use QTextEdit
#include <QTextEdit>
#include <QUrl>
//#else
//#include <Q3TextEdit>
//#include <Q3Url>
//#endif
//Added by qt3to4:
#include <QCloseEvent>
#include <QKeyEvent>
#include <cstdlib>
#include <iostream>
#include <cmath>
#include "appWindow.h"
#include "chooseFilesDlg.h"
#include "polyView.h"
#include "utils.h"

using namespace std;

#ifdef ADD_PREF_INI
#ifndef DEF_WIDTH
#define DEF_WIDTH 900
#endif
#endif // ADD_PREF_INI

cmdLine::cmdLine(QWidget* parent): QLineEdit(parent){}
cmdLine::~cmdLine(){}

#ifdef USE_QT4_DEFS // Q3PopupWindow, Q3MainWindo... subs
appWindow::appWindow(QWidget* parent, std::string progName,
                     const cmdLineOptions & options, 
                     int windowWidX, int windowWidY
                     ) : 
    Q3MainWindow(parent, progName.c_str()), m_poly(NULL)
#else 
appWindow::appWindow(QWidget* parent, std::string progName,
                     const cmdLineOptions & options, 
                     int windowWidX, int windowWidY
                     ) : 
    QMainWindow(parent), m_poly(NULL)
#endif
{

  installEventFilter(this);

  m_progName = progName;
  resize(windowWidX, windowWidY);

  createMenusAndMainWidget(options);
  installEventFilter(m_poly);
  
  // Command line
  m_cmdLine = new cmdLine(this);
  m_cmdLine->setAlignment(Qt::AlignLeft);
  m_cmdLine->setFocusPolicy(Qt::StrongFocus);
  connect( m_cmdLine, SIGNAL( returnPressed() ),
           this, SLOT( procCmdLine() ) );
  QStatusBar * status = statusBar();
  QRect Rp = status->rect();
  m_cmdLine->setGeometry(Rp);
  status->addWidget(m_cmdLine, 1);
  m_cmdHist.clear();
  m_histPos = 0;

  return;
}

void appWindow::closeEvent(QCloseEvent *){
  forceQuit();
}

void appWindow::forceQuit()
{
#ifdef ADD_PREF_INI
    if (m_poly) {
        m_poly->writeINI();
        //utils::writeINI(m_poly->getOptions());
    }
#endif // ADD_PREF_INI
  exit(0); // A fix for an older buggy version of Qt
}
                     
bool appWindow::eventFilter(QObject *obj, QEvent *event){

  // If the alt or control key is hit, and the focus is on the command
  // line widget, move the focus to the m_poly widget.
  if ( event->type() == QEvent::KeyPress ||
       event->type() == QEvent::ShortcutOverride){
    QKeyEvent * keyEvent = (QKeyEvent*)event;
    Qt::KeyboardModifiers modifiers = keyEvent->modifiers();
    if ( ( modifiers & Qt::ControlModifier ) ||
         ( modifiers & Qt::AltModifier ) ){
      if (m_cmdLine->hasFocus() && m_poly != NULL) m_poly->setFocus();
    }
  }

  if (obj == m_poly) {
    // Avoid repainting on these events
    if (event->type() == QEvent::FocusIn          ||
        event->type() == QEvent::FocusOut         ||
        event->type() == QEvent::WindowDeactivate ||
        event->type() == QEvent::Leave
        ){
      return true;
    }
  }
  
  //cout << "Other event: " << (int)event->type() << endl;
  return QWidget::eventFilter(obj, event);
}

appWindow::~appWindow(){

  if (m_poly != NULL){ delete m_poly; m_poly = NULL; }
  
}

void appWindow::procCmdLine(){
  string cmd = m_cmdLine->text().toStdString();
  m_cmdHist.push_back(cmd);
  m_poly->runCmd(cmd);
  m_cmdLine->setText("");
  m_histPos = m_cmdHist.size();
  //m_poly->setFocus();
}

void appWindow::insertCmdFromHist(){

  int numHistItems = m_cmdHist.size();
  m_histPos = min(m_histPos, numHistItems);
  m_histPos = max(0, m_histPos);
  
  if (m_histPos < numHistItems){
    m_cmdLine->setText(m_cmdHist[m_histPos].c_str());
  }else{
    m_cmdLine->setText("");
  }

  return;
}

void appWindow::shiftUp (){

  if (m_poly->hasFocus()){
    m_poly->shiftUp ();
  }else if(m_cmdLine->hasFocus()){
    m_histPos--;
    insertCmdFromHist();
  }
  
}

void appWindow::shiftDown (){

  if (m_poly->hasFocus()){
    m_poly->shiftDown ();
  }else if(m_cmdLine->hasFocus()){
    m_histPos++;
    insertCmdFromHist();
  }
  
}

void appWindow::createMenusAndMainWidget(const cmdLineOptions & opt){

  // There is some twisted logic here. First initialize the menus,
  // then create the main widget, then finish creating the menus.
  // This is a workaround for a bug in a certain versions of Qt. If
  // the main widget is created before the menus then it gets
  // incorrect geometry.
  
  QMenuBar* menu = menuBar();
#ifdef USE_QT4_DEFS
  Q3PopupMenu* file = new Q3PopupMenu( menu );
#else
  //QMenu * file = new QMenu( menu );
#endif

  // Central widget
  m_poly = new polyView (this, opt);
  m_poly->setFocusPolicy(Qt::StrongFocus);
  m_poly->setFocus();
  setCentralWidget(m_poly);

#ifdef USE_QT4_DEFS
  menu->insertItem("File", file);

  file->insertItem("Open", m_poly, SLOT(openPoly()), Qt::CTRL+Qt::Key_O);
  file->insertItem("Save as a combined file", m_poly, SLOT(saveOnePoly()),
                   Qt::CTRL+Qt::Key_S);
  file->insertItem("Save as individual files", m_poly,
                   SLOT(saveAsMultiplePolys()), Qt::ALT+Qt::Key_S);
  file->insertItem("Overwrite current files", m_poly,
                   SLOT(overwriteMultiplePolys()), Qt::CTRL+Qt::Key_W);
#ifdef ADD_IMAGE_SAVE
  file->insertItem("Save as image file", m_poly,
                   SLOT(saveScreenImage()), Qt::ALT+Qt::Key_I);
#endif // ADD_IMAGE_SAVE
  file->insertItem("Exit", this, SLOT(forceQuit()), Qt::Key_Q);

  Q3PopupMenu* view = new Q3PopupMenu( menu );
  menu->insertItem("View", view);
  //view->insertSeparator();
  view->insertItem(chooseFilesDlg::selectFilesTag(), m_poly, SLOT(chooseFilesToShow()));
  view->insertItem("Zoom out",             m_poly, SLOT(zoomOut()),      Qt::Key_Minus);
  view->insertItem("Zoom in",              m_poly, SLOT(zoomIn()),       Qt::Key_Equal);
  view->insertItem("Move left",            m_poly, SLOT(shiftLeft()),    Qt::Key_Left);
  view->insertItem("Move right",           m_poly, SLOT(shiftRight()),   Qt::Key_Right);
  view->insertItem("Move up",              this,   SLOT(shiftUp()),      Qt::Key_Up);
  view->insertItem("Move down",            this,   SLOT(shiftDown()),    Qt::Key_Down);
  view->insertItem("Reset view",           m_poly, SLOT(resetView()),    Qt::Key_R);
  view->insertItem("Change display order", m_poly, SLOT(changeOrder()),  Qt::Key_O);
  view->insertItem("Toggle annotations",   m_poly, SLOT(toggleAnno()),   Qt::Key_A);
  view->insertItem("Toggle filled",        m_poly, SLOT(toggleFilled()), Qt::Key_F);
  view->insertItem("Toggle points display",
                   m_poly, SLOT(togglePE()),          Qt::Key_P);
  view->insertItem("Toggle show vertex indices",
                   m_poly, SLOT(toggleVertIndexAnno()), Qt::Key_V);
  view->insertItem("Toggle show layers", m_poly, SLOT(toggleLayerAnno()), Qt::Key_L);

  Q3PopupMenu* edit = new Q3PopupMenu( menu );
  menu->insertItem("Edit", edit);
  edit->insertItem("Undo",  m_poly, SLOT(undo()), Qt::Key_Z);
  edit->insertItem("Redo",  m_poly, SLOT(redo()), Qt::ALT + Qt::Key_Z);
  
  edit->insertItem("Create poly with int vertices and 45x deg angles",
                   m_poly, SLOT(create45DegreeIntPoly()), Qt::Key_N);
  edit->insertItem("Enforce int vertices and 45x deg angles", m_poly, SLOT(enforce45()),
                   Qt::CTRL+Qt::Key_4);
  edit->insertItem("Create arbitrary polygon",
                   m_poly, SLOT(createArbitraryPoly()), Qt::CTRL+Qt::Key_N);
  edit->insertItem("Merge polygons (buggy)",
                   m_poly, SLOT(mergePolys()), Qt::CTRL+Qt::Key_M);

  Q3PopupMenu* selection = new Q3PopupMenu( menu );
  menu->insertItem("Selection", selection);
  selection->insertItem("Create highlight", m_poly, SLOT(createHlt()));
  selection->insertItem("Cut polygons to highlight",
                        m_poly, SLOT(cutToHlt()), Qt::Key_C);
  selection->insertItem("Delete selected polygons",
                        m_poly, SLOT(deleteSelectedPolys()), Qt::CTRL+Qt::Key_D);
  selection->insertItem("Paste selected polygons",
                        m_poly, SLOT(pasteSelectedPolys()), Qt::CTRL+Qt::Key_V);
  selection->insertItem("Move selected polygons",
                        m_poly, SLOT(moveSelectedPolys()));
  selection->insertItem("Deselect polygons/delete highlights",  m_poly,
                        SLOT(deselectPolysDeleteHlts()));

  Q3PopupMenu* transform = new Q3PopupMenu( menu );
  menu->insertItem("Transform", transform);
  transform->insertItem("Translate selected polygons", m_poly,
                        SLOT(translateSelectedPolys()), Qt::CTRL+Qt::Key_T );
  transform->insertItem("Rotate selected polygons",    m_poly,
                        SLOT(rotateSelectedPolys()), Qt::CTRL+Qt::Key_R );
  transform->insertItem("Scale selected polygons",
                        m_poly, SLOT(scaleSelectedPolys()) );
  transform->insertItem("Transform selected polygons", m_poly,
                        SLOT(transformSelectedPolys()) );
#if 0
  selection->addSeparator();
  transform->insertItem("Translate polygons", m_poly, SLOT(translatePolys()),
                        Qt::CTRL+Qt::Key_T);
  transform->insertItem("Rotate polygons around origin", m_poly, SLOT(rotatePolys()),
                        Qt::CTRL+Qt::Key_R);
  transform->insertItem("Scale polygons around origin", m_poly, SLOT(scalePolys()),
                        Qt::CTRL+Qt::Key_X);
#endif

  Q3PopupMenu* grid = new Q3PopupMenu( menu );
  menu->insertItem("Grid", grid);
  grid->insertItem("Toggle poly grid", m_poly, SLOT(toggleShowGrid()), Qt::Key_G);
  grid->insertItem("Enforce 45x deg angles and snap to grid", m_poly, SLOT(enforce45AndSnapToGrid()));
  grid->insertItem("Set grid size", m_poly, SLOT(setGridSize()));
  grid->insertItem("Set grid linewidth", m_poly, SLOT(setGridWidth()));
  grid->insertItem("Set grid color", m_poly, SLOT(setGridColor()));

  Q3PopupMenu* diff = new Q3PopupMenu( menu );
  menu->insertItem("Diff", diff);
  diff->insertItem("Toggle different colors", m_poly, SLOT(toggleDifferentColors()), Qt::SHIFT + Qt::Key_D);
  diff->insertItem("Toggle show poly diff", m_poly, SLOT(toggleShowPolyDiff()), Qt::Key_D);
  diff->insertItem("Show next diff", m_poly, SLOT(plotNextDiff()), Qt::Key_K);
  diff->insertItem("Show prev diff", m_poly, SLOT(plotPrevDiff()), Qt::Key_J);

  Q3PopupMenu* options = new Q3PopupMenu( menu );
  menu->insertItem("Options", options);
  options->insertItem("Set line width", m_poly, SLOT(setLineWidth()));
  options->insertItem("Set background color", m_poly, SLOT(setBgColor()));

  Q3PopupMenu* help = new Q3PopupMenu( menu );
  menu->insertItem("Help", help);
  help->insertItem("Show documentation", this, SLOT(showDoc()));
  help->insertItem("About", this, SLOT(about()));
#else
// [1] File
  fileMenu = menu->addMenu(tr("&File"));
// [1.1] Open
  //file->insertItem("Open", m_poly, SLOT(openPoly()), Qt::CTRL+Qt::Key_O);
  openAct = new QAction(tr("&Open..."), this);
  openAct->setStatusTip(tr("Open an existing file"));
  openAct->setShortcut(Qt::CTRL+Qt::Key_O);
  connect(openAct, SIGNAL(triggered()), m_poly, SLOT(openPoly()));

// [1.2] Save One
  // file->insertItem("Save as a combined file", m_poly, SLOT(saveOnePoly()), Qt::CTRL+Qt::Key_S);
  saveoneAct = new QAction(tr("&Save as One..."), this);
  saveoneAct->setStatusTip(tr("Save as a combined file"));
  saveoneAct->setShortcut(Qt::CTRL+Qt::Key_S);
  connect(saveoneAct, SIGNAL(triggered()), m_poly, SLOT(saveOnePoly()));

// [1.3] Save Many
  //file->insertItem("Save as individual files", m_poly, SLOT(saveAsMultiplePolys()), Qt::ALT+Qt::Key_S);
  savemultAct = new QAction(tr("S&ave as Many..."), this);
  savemultAct->setStatusTip(tr("Save as individual files"));
  savemultAct->setShortcut(Qt::ALT+Qt::Key_S);
  connect(savemultAct, SIGNAL(triggered()), m_poly, SLOT(saveAsMultiplePolys()));

  // file->insertItem("Overwrite current files", m_poly, SLOT(overwriteMultiplePolys()), Qt::CTRL+Qt::Key_W);
  QAction *writeAct = new QAction(tr("&Write now..."), this);
  writeAct->setStatusTip(tr("Overwrite current files"));
  writeAct->setShortcut(Qt::CTRL+Qt::Key_W);
  connect(writeAct, SIGNAL(triggered()), m_poly, SLOT(overwriteMultiplePolys()));

#ifdef ADD_IMAGE_SAVE
  // file->insertItem("Save as image file", m_poly, SLOT(saveScreenImage()), Qt::ALT+Qt::Key_I);
  QAction *imageAct = new QAction(tr("&Image..."),this);
  imageAct->setStatusTip(tr("Save as image file"));
  imageAct->setShortcut(Qt::ALT+Qt::Key_I);
  connect(imageAct, SIGNAL(triggered()), m_poly, SLOT(saveScreenImage()));
#endif // ADD_IMAGE_SAVE

// [1.6] Exit
  exitAct = new QAction(tr("E&xit"), this);
  //exitAct->setShortcuts(QKeySequence::Quit);
  exitAct->setShortcut(Qt::Key_Q);
  exitAct->setStatusTip(tr("Exit the application"));
  connect(exitAct, SIGNAL(triggered()), this, SLOT(forceQuit()));

  fileMenu->addAction(openAct);
  fileMenu->addAction(saveoneAct);
  fileMenu->addAction(savemultAct);
  fileMenu->addAction(writeAct);
#ifdef ADD_IMAGE_SAVE
  fileMenu->addAction(imageAct);
#endif
  fileMenu->addAction(exitAct);

// #### GEN CODE ############################################## 
// gen Qt5 code, gen by qt42qt5.pl, on 2016/06/21 00:31:55 UTC.

// Start a NEW Menu item
  viewMenu = menu->addMenu(tr("View"));
  chooseFilesToShowAct = new QAction(tr("Select Files show/hide"),this);
  chooseFilesToShowAct->setStatusTip(tr("Select Files show/hide"));
  connect(chooseFilesToShowAct, SIGNAL(triggered()), m_poly, SLOT(chooseFilesToShow()));

  zoomOutAct = new QAction(tr("Zoom out"),this);
  zoomOutAct->setStatusTip(tr("Zoom out"));
  zoomOutAct->setShortcut(Qt::Key_Minus);
  connect(zoomOutAct, SIGNAL(triggered()), m_poly, SLOT(zoomOut()));

  zoomInAct = new QAction(tr("Zoom in"),this);
  zoomInAct->setStatusTip(tr("Zoom in"));
  zoomInAct->setShortcut(Qt::Key_Equal);
  connect(zoomInAct, SIGNAL(triggered()), m_poly, SLOT(zoomIn()));

  shiftLeftAct = new QAction(tr("Move left"),this);
  shiftLeftAct->setStatusTip(tr("Move left"));
  shiftLeftAct->setShortcut(Qt::Key_Left);
  connect(shiftLeftAct, SIGNAL(triggered()), m_poly, SLOT(shiftLeft()));

  shiftRightAct = new QAction(tr("Move right"),this);
  shiftRightAct->setStatusTip(tr("Move right"));
  shiftRightAct->setShortcut(Qt::Key_Right);
  connect(shiftRightAct, SIGNAL(triggered()), m_poly, SLOT(shiftRight()));

  shiftUpAct = new QAction(tr("Move up"),this);
  shiftUpAct->setStatusTip(tr("Move up"));
  shiftUpAct->setShortcut(Qt::Key_Up);
  connect(shiftUpAct, SIGNAL(triggered()), this, SLOT(shiftUp()));

  shiftDownAct = new QAction(tr("Move down"),this);
  shiftDownAct->setStatusTip(tr("Move down"));
  shiftDownAct->setShortcut(Qt::Key_Down);
  connect(shiftDownAct, SIGNAL(triggered()), this, SLOT(shiftDown()));

  resetViewAct = new QAction(tr("Reset view"),this);
  resetViewAct->setStatusTip(tr("Reset view"));
  resetViewAct->setShortcut(Qt::Key_R);
  connect(resetViewAct, SIGNAL(triggered()), m_poly, SLOT(resetView()));

  changeOrderAct = new QAction(tr("Change display order"),this);
  changeOrderAct->setStatusTip(tr("Change display order"));
  changeOrderAct->setShortcut(Qt::Key_O);
  connect(changeOrderAct, SIGNAL(triggered()), m_poly, SLOT(changeOrder()));

  toggleAnnoAct = new QAction(tr("Toggle annotations"),this);
  toggleAnnoAct->setStatusTip(tr("Toggle annotations"));
  toggleAnnoAct->setShortcut(Qt::Key_A);
  connect(toggleAnnoAct, SIGNAL(triggered()), m_poly, SLOT(toggleAnno()));

  toggleFilledAct = new QAction(tr("Toggle filled"),this);
  toggleFilledAct->setStatusTip(tr("Toggle filled"));
  toggleFilledAct->setShortcut(Qt::Key_F);
  connect(toggleFilledAct, SIGNAL(triggered()), m_poly, SLOT(toggleFilled()));

  togglePEAct = new QAction(tr("Toggle points display"),this);
  togglePEAct->setStatusTip(tr("Toggle points display"));
  togglePEAct->setShortcut(Qt::Key_P);
  connect(togglePEAct, SIGNAL(triggered()), m_poly, SLOT(togglePE()));

  toggleVertIndexAnnoAct = new QAction(tr("Toggle show vertex indices"),this);
  toggleVertIndexAnnoAct->setStatusTip(tr("Toggle show vertex indices"));
  toggleVertIndexAnnoAct->setShortcut(Qt::Key_V);
  connect(toggleVertIndexAnnoAct, SIGNAL(triggered()), m_poly, SLOT(toggleVertIndexAnno()));

  toggleLayerAnnoAct = new QAction(tr("Toggle show layers"),this);
  toggleLayerAnnoAct->setStatusTip(tr("Toggle show layers"));
  toggleLayerAnnoAct->setShortcut(Qt::Key_L);
  connect(toggleLayerAnnoAct, SIGNAL(triggered()), m_poly, SLOT(toggleLayerAnno()));


// add 14 actions to menu viewMenu
  viewMenu->addAction(chooseFilesToShowAct);
  viewMenu->addAction(zoomOutAct);
  viewMenu->addAction(zoomInAct);
  viewMenu->addAction(shiftLeftAct);
  viewMenu->addAction(shiftRightAct);
  viewMenu->addAction(shiftUpAct);
  viewMenu->addAction(shiftDownAct);
  viewMenu->addAction(resetViewAct);
  viewMenu->addAction(changeOrderAct);
  viewMenu->addAction(toggleAnnoAct);
  viewMenu->addAction(toggleFilledAct);
  viewMenu->addAction(togglePEAct);
  viewMenu->addAction(toggleVertIndexAnnoAct);
  viewMenu->addAction(toggleLayerAnnoAct);

// Start a NEW Menu item
  editMenu = menu->addMenu(tr("Edit"));
  undoAct = new QAction(tr("Undo"),this);
  undoAct->setStatusTip(tr("Undo"));
  undoAct->setShortcut(Qt::Key_Z);
  connect(undoAct, SIGNAL(triggered()), m_poly, SLOT(undo()));

  redoAct = new QAction(tr("Redo"),this);
  redoAct->setStatusTip(tr("Redo"));
  redoAct->setShortcut(Qt::ALT + Qt::Key_Z);
  connect(redoAct, SIGNAL(triggered()), m_poly, SLOT(redo()));

  create45DegreeIntPolyAct = new QAction(tr("Create poly with int vertices and 45x deg angles"),this);
  create45DegreeIntPolyAct->setStatusTip(tr("Create poly with int vertices and 45x deg angles"));
  create45DegreeIntPolyAct->setShortcut(Qt::Key_N);
  connect(create45DegreeIntPolyAct, SIGNAL(triggered()), m_poly, SLOT(create45DegreeIntPoly()));

  enforce45Act = new QAction(tr("Enforce int vertices and 45x deg angles"),this);
  enforce45Act->setStatusTip(tr("Enforce int vertices and 45x deg angles"));
  enforce45Act->setShortcut(Qt::CTRL+Qt::Key_4);
  connect(enforce45Act, SIGNAL(triggered()), m_poly, SLOT(enforce45()));

  createArbitraryPolyAct = new QAction(tr("Create arbitrary polygon"),this);
  createArbitraryPolyAct->setStatusTip(tr("Create arbitrary polygon"));
  createArbitraryPolyAct->setShortcut(Qt::CTRL+Qt::Key_N);
  connect(createArbitraryPolyAct, SIGNAL(triggered()), m_poly, SLOT(createArbitraryPoly()));

  mergePolysAct = new QAction(tr("Merge polygons (buggy)"),this);
  mergePolysAct->setStatusTip(tr("Merge polygons (buggy)"));
  mergePolysAct->setShortcut(Qt::CTRL+Qt::Key_M);
  connect(mergePolysAct, SIGNAL(triggered()), m_poly, SLOT(mergePolys()));


// add 6 actions to menu editMenu
  editMenu->addAction(undoAct);
  editMenu->addAction(redoAct);
  editMenu->addAction(create45DegreeIntPolyAct);
  editMenu->addAction(enforce45Act);
  editMenu->addAction(createArbitraryPolyAct);
  editMenu->addAction(mergePolysAct);

// Start a NEW Menu item
  selectionMenu = menu->addMenu(tr("Selection"));
  createHltAct = new QAction(tr("Create highlight"),this);
  createHltAct->setStatusTip(tr("Create highlight"));
  connect(createHltAct, SIGNAL(triggered()), m_poly, SLOT(createHlt()));

  cutToHltAct = new QAction(tr("Cut polygons to highlight"),this);
  cutToHltAct->setStatusTip(tr("Cut polygons to highlight"));
  cutToHltAct->setShortcut(Qt::Key_C);
  connect(cutToHltAct, SIGNAL(triggered()), m_poly, SLOT(cutToHlt()));

  deleteSelectedPolysAct = new QAction(tr("Delete selected polygons"),this);
  deleteSelectedPolysAct->setStatusTip(tr("Delete selected polygons"));
  deleteSelectedPolysAct->setShortcut(Qt::CTRL+Qt::Key_D);
  connect(deleteSelectedPolysAct, SIGNAL(triggered()), m_poly, SLOT(deleteSelectedPolys()));

  pasteSelectedPolysAct = new QAction(tr("Paste selected polygons"),this);
  pasteSelectedPolysAct->setStatusTip(tr("Paste selected polygons"));
  pasteSelectedPolysAct->setShortcut(Qt::CTRL+Qt::Key_V);
  connect(pasteSelectedPolysAct, SIGNAL(triggered()), m_poly, SLOT(pasteSelectedPolys()));

  moveSelectedPolysAct = new QAction(tr("Move selected polygons"),this);
  moveSelectedPolysAct->setStatusTip(tr("Move selected polygons"));
  connect(moveSelectedPolysAct, SIGNAL(triggered()), m_poly, SLOT(moveSelectedPolys()));

  deselectPolysDeleteHltsAct = new QAction(tr("Deselect polygons/delete highlights"),this);
  deselectPolysDeleteHltsAct->setStatusTip(tr("Deselect polygons/delete highlights"));
  connect(deselectPolysDeleteHltsAct, SIGNAL(triggered()), m_poly, SLOT(deselectPolysDeleteHlts()));


// add 6 actions to menu selectionMenu
  selectionMenu->addAction(createHltAct);
  selectionMenu->addAction(cutToHltAct);
  selectionMenu->addAction(deleteSelectedPolysAct);
  selectionMenu->addAction(pasteSelectedPolysAct);
  selectionMenu->addAction(moveSelectedPolysAct);
  selectionMenu->addAction(deselectPolysDeleteHltsAct);

// Start a NEW Menu item
  transformMenu = menu->addMenu(tr("Transform"));
  translateSelectedPolysAct = new QAction(tr("Translate selected polygons"),this);
  translateSelectedPolysAct->setStatusTip(tr("Translate selected polygons"));
  translateSelectedPolysAct->setShortcut(Qt::CTRL+Qt::Key_T );
  connect(translateSelectedPolysAct, SIGNAL(triggered()), m_poly, SLOT(translateSelectedPolys()));

  rotateSelectedPolysAct = new QAction(tr("Rotate selected polygons"),this);
  rotateSelectedPolysAct->setStatusTip(tr("Rotate selected polygons"));
  rotateSelectedPolysAct->setShortcut(Qt::CTRL+Qt::Key_R );
  connect(rotateSelectedPolysAct, SIGNAL(triggered()), m_poly, SLOT(rotateSelectedPolys()));

  scaleSelectedPolysAct = new QAction(tr("Scale selected polygons"),this);
  scaleSelectedPolysAct->setStatusTip(tr("Scale selected polygons"));
  connect(scaleSelectedPolysAct, SIGNAL(triggered()), m_poly, SLOT(scaleSelectedPolys()));

  transformSelectedPolysAct = new QAction(tr("Transform selected polygons"),this);
  transformSelectedPolysAct->setStatusTip(tr("Transform selected polygons"));
  connect(transformSelectedPolysAct, SIGNAL(triggered()), m_poly, SLOT(transformSelectedPolys()));

  translatePolysAct = new QAction(tr("Translate polygons"),this);
  translatePolysAct->setStatusTip(tr("Translate polygons"));
  translatePolysAct->setShortcut(Qt::CTRL+Qt::Key_T);
  connect(translatePolysAct, SIGNAL(triggered()), m_poly, SLOT(translatePolys()));

  rotatePolysAct = new QAction(tr("Rotate polygons around origin"),this);
  rotatePolysAct->setStatusTip(tr("Rotate polygons around origin"));
  rotatePolysAct->setShortcut(Qt::CTRL+Qt::Key_R);
  connect(rotatePolysAct, SIGNAL(triggered()), m_poly, SLOT(rotatePolys()));

  scalePolysAct = new QAction(tr("Scale polygons around origin"),this);
  scalePolysAct->setStatusTip(tr("Scale polygons around origin"));
  scalePolysAct->setShortcut(Qt::CTRL+Qt::Key_X);
  connect(scalePolysAct, SIGNAL(triggered()), m_poly, SLOT(scalePolys()));


// add 7 actions to menu transformMenu
  transformMenu->addAction(translateSelectedPolysAct);
  transformMenu->addAction(rotateSelectedPolysAct);
  transformMenu->addAction(scaleSelectedPolysAct);
  transformMenu->addAction(transformSelectedPolysAct);
  transformMenu->addAction(translatePolysAct);
  transformMenu->addAction(rotatePolysAct);
  transformMenu->addAction(scalePolysAct);

// Start a NEW Menu item
  gridMenu = menu->addMenu(tr("Grid"));
  toggleShowGridAct = new QAction(tr("Toggle poly grid"),this);
  toggleShowGridAct->setStatusTip(tr("Toggle poly grid"));
  toggleShowGridAct->setShortcut(Qt::Key_G);
  connect(toggleShowGridAct, SIGNAL(triggered()), m_poly, SLOT(toggleShowGrid()));

  enforce45AndSnapToGridAct = new QAction(tr("Enforce 45x deg angles and snap to grid"),this);
  enforce45AndSnapToGridAct->setStatusTip(tr("Enforce 45x deg angles and snap to grid"));
  connect(enforce45AndSnapToGridAct, SIGNAL(triggered()), m_poly, SLOT(enforce45AndSnapToGrid()));

  setGridSizeAct = new QAction(tr("Set grid size"),this);
  setGridSizeAct->setStatusTip(tr("Set grid size"));
  connect(setGridSizeAct, SIGNAL(triggered()), m_poly, SLOT(setGridSize()));

  setGridWidthAct = new QAction(tr("Set grid linewidth"),this);
  setGridWidthAct->setStatusTip(tr("Set grid linewidth"));
  connect(setGridWidthAct, SIGNAL(triggered()), m_poly, SLOT(setGridWidth()));

  setGridColorAct = new QAction(tr("Set grid color"),this);
  setGridColorAct->setStatusTip(tr("Set grid color"));
  connect(setGridColorAct, SIGNAL(triggered()), m_poly, SLOT(setGridColor()));


// add 5 actions to menu gridMenu
  gridMenu->addAction(toggleShowGridAct);
  gridMenu->addAction(enforce45AndSnapToGridAct);
  gridMenu->addAction(setGridSizeAct);
  gridMenu->addAction(setGridWidthAct);
  gridMenu->addAction(setGridColorAct);

// Start a NEW Menu item
  diffMenu = menu->addMenu(tr("Diff"));
  toggleDifferentColorsAct = new QAction(tr("Toggle different colors"),this);
  toggleDifferentColorsAct->setStatusTip(tr("Toggle different colors"));
  toggleDifferentColorsAct->setShortcut(Qt::SHIFT + Qt::Key_D);
  connect(toggleDifferentColorsAct, SIGNAL(triggered()), m_poly, SLOT(toggleDifferentColors()));

  toggleShowPolyDiffAct = new QAction(tr("Toggle show poly diff"),this);
  toggleShowPolyDiffAct->setStatusTip(tr("Toggle show poly diff"));
  toggleShowPolyDiffAct->setShortcut(Qt::Key_D);
  connect(toggleShowPolyDiffAct, SIGNAL(triggered()), m_poly, SLOT(toggleShowPolyDiff()));

  plotNextDiffAct = new QAction(tr("Show next diff"),this);
  plotNextDiffAct->setStatusTip(tr("Show next diff"));
  plotNextDiffAct->setShortcut(Qt::Key_K);
  connect(plotNextDiffAct, SIGNAL(triggered()), m_poly, SLOT(plotNextDiff()));

  plotPrevDiffAct = new QAction(tr("Show prev diff"),this);
  plotPrevDiffAct->setStatusTip(tr("Show prev diff"));
  plotPrevDiffAct->setShortcut(Qt::Key_J);
  connect(plotPrevDiffAct, SIGNAL(triggered()), m_poly, SLOT(plotPrevDiff()));


// add 4 actions to menu diffMenu
  diffMenu->addAction(toggleDifferentColorsAct);
  diffMenu->addAction(toggleShowPolyDiffAct);
  diffMenu->addAction(plotNextDiffAct);
  diffMenu->addAction(plotPrevDiffAct);

// Start a NEW Menu item
  optionsMenu = menu->addMenu(tr("Options"));
  setLineWidthAct = new QAction(tr("Set line width"),this);
  setLineWidthAct->setStatusTip(tr("Set line width"));
  connect(setLineWidthAct, SIGNAL(triggered()), m_poly, SLOT(setLineWidth()));

  setBgColorAct = new QAction(tr("Set background color"),this);
  setBgColorAct->setStatusTip(tr("Set background color"));
  connect(setBgColorAct, SIGNAL(triggered()), m_poly, SLOT(setBgColor()));


// add 2 actions to menu optionsMenu
  optionsMenu->addAction(setLineWidthAct);
  optionsMenu->addAction(setBgColorAct);

// Start a NEW Menu item
  helpMenu = menu->addMenu(tr("Help"));
  showDocAct = new QAction(tr("Show documentation"),this);
  showDocAct->setStatusTip(tr("Show documentation"));
  connect(showDocAct, SIGNAL(triggered()), this, SLOT(showDoc()));

  aboutAct = new QAction(tr("About"),this);
  aboutAct->setStatusTip(tr("About"));
  connect(aboutAct, SIGNAL(triggered()), this, SLOT(about()));


// add 2 actions to menu helpMenu
  helpMenu->addAction(showDocAct);
  helpMenu->addAction(aboutAct);

// #### END GEN CODE ############################################## 

#endif
  return;
}

void appWindow::showDoc(){
  m_docWindow.setText(utils::getDocText());
#ifdef USE_QT4_DEFS // ref setCation
  m_docWindow.setCaption(this->caption()); // Borrow the caption from the parent
#endif
  m_docWindow.show();
  return;
}

void appWindow::about(){

  string aboutStr = string("About ") + m_progName;
#ifdef USE_QT4_DEFS
  static QMessageBox* about
    = new QMessageBox( aboutStr.c_str(),
                       "     Oleg Alexandrov        ", // extra space to make window bigger
                       QMessageBox::NoIcon, 1, 0, 0, this, 0, false );
  about->setButtonText( 1, "OK" );
  about->show();
#else
    QMessageBox::about(this, tr("About PolyView-2D"),
            tr("<b>PolyView-2D</b> Version: " PV2D_VERSION " (" PV2D_DATE ")<br>"
            "It will load a 'poly' file, (.xg...), for display, and editing.\n"
            "See Menu - Show Document view for more information.\n<br>"
            "Brought to you by Oleg Alexandrov and Geoff R. McLane.\n "
               ));
#endif
  return;
}

docWindow::docWindow(QWidget *) {
  resize(900, 800);

#ifdef _MSC_VER
  QIcon icon(":/pv.ico"); 
  setWindowIcon(icon);
#endif

  m_textArea = new QTextEdit(this); 

  setCentralWidget (m_textArea); 

}

docWindow::~docWindow(){
  delete m_textArea;
  m_textArea = NULL;
}

void docWindow::setText(const std::string & docText){

  QImage img(":/pvLogo.png");
//#ifdef _MSC_VER // drop Q3TextEdit, use QTextEdit, and Q3Url, use QUrl
  m_textArea->document()->addResource(QTextDocument::ImageResource,
                                      QUrl("pvLogo.png" ), img);
//#else
//  m_textArea->document()->addResource(QTextDocument::ImageResource,
//                                      Q3Url("pvLogo.png" ), img);
//#endif
  m_textArea->clear();
  m_textArea->setReadOnly(true);
  m_textArea->setCurrentFont(QFont("Monospace", 10)); 
  m_textArea->insertHtml(docText.c_str());
  m_textArea->moveCursor(QTextCursor::Start);
  
  return;
}
