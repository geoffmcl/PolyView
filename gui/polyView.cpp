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
#ifdef USE_QT4_DEFS // Q3PointArray, and ... subs
#include <Q3PointArray>
#include <Q3PopupMenu>
#include <Q3FileDialog>
#else
// #include <QPopupMenu>
#include <QMenu>
#endif

#include <QContextMenuEvent>
#include <QEvent>
#ifdef ADD_IMAGE_SAVE
#include <QFileDialog>
#endif // ADD_IMAGE_SAVE

#include <QHoverEvent>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QStyleOptionFocusRect>
#include <QStylePainter>
#include <QTableWidget>
#include <QWheelEvent>
//Added by qt3to4:
#include <QResizeEvent>
#include <QPixmap>
#include <cassert>
#include <cfloat>    // defines DBL_MAX
#include <cmath>
#include <cstdlib>
#include <iomanip>   // required for use of setw()
#include <iostream>
#include <qapplication.h>
#include <qcursor.h>
#include <qdir.h>
#include <qinputdialog.h>
#include <qpainter.h>
#include "polyView.h"
#include <qmessagebox.h>
#ifdef WIN32
#include <direct.h> // for _getcwd
#define getcwd _getcwd
#endif  // WIN32
#include "utils.h"

#define KM2NM 0.53996

using namespace std;
using namespace utils;

// To do: handle colors correctly (convert dark-gray to darkGray, etc.).
// To do: In the geom directory, put everything in a namespace, say called 'pv'.
//        Here too. Clean up, modularize, and structure the code more.
// To do: Fix other "To do" mentioned in the code.
// To do: The viewer does not render correctly in fill mode overlapping polygons
//        with each polygon having holes. A fix would require a thorough analysis
//        which would identify which hole belongs to which polygon.
// To do: Replace cmdLineOptions directly with polyOptionsVec.
// To do: Make font size a preference.

polyView::polyView(QWidget *parent, const cmdLineOptions & options): QWidget(parent){

    m_parent = parent;
  // Choose which files to hide/show in the GUI
  QObject::connect(m_chooseFilesDlg.getFilesTable(),
                   SIGNAL(itemSelectionChanged()),
                   this,
                   SLOT(showFilesChosenByUser())
                   );
    
  setAttribute(Qt::WA_Hover); // To be able to do hovering
  
  // Preferences per polygon file. The element in the vector
  // m_polyOptionsVec below is not associated with any polygon
  // file. Set it apart, it will be used for new polygons.
  m_polyOptionsVec = options.polyOptionsVec;
  assert(m_polyOptionsVec.size() >= 1);
  m_prefs = m_polyOptionsVec.back(); m_polyOptionsVec.pop_back();
  m_prefs.plotAsPoints = false; // most likely the user wants to see edges not points
  setBgFgColorsFromPrefs(); // must be called after m_prefs is set
  
  setStandardCursor();

  // int
  m_screenXll  = 0; m_screenYll  = 0;
  m_screenWidX = 0; m_screenWidY = 0;

  // double
  m_viewXll  = 0.0; m_viewYll  = 0.0;
  m_viewWidX = 0.0; m_viewWidY = 0.0;

  m_resetView       = true;
  m_prevClickExists = false;
  m_firstPaintEvent = true;
  
  m_showAnnotations    = true;
  m_showVertIndexAnno  = false;
  m_showLayerAnno      = false;
  m_showFilledPolys    = false;
  m_changeDisplayOrder = false;
  
  m_emptyRubberBand = QRect(-10, -10, 0, 0); // off-screen rubberband
  m_rubberBand      = m_emptyRubberBand;

  m_showEdges               = 1;
  m_showPointsEdges         = 2;
  m_showPoints              = 3;
  m_toggleShowPointsEdges   = m_showEdges;

  m_createPoly                = false;
  m_snapPolyTo45DegreeIntGrid = false;
  m_currPolyX.clear(); m_currPolyY.clear();

  m_zoomToMouseSelection = false;
  m_viewChanged          = false;

  m_zoomFactor = 1.0;
  m_mousePrsX = 0; m_mousePrsY = 0;
  m_mouseRelX = 0; m_mouseRelY = 0;
  
  // Points closer than this are in some situations considered equal
  m_pixelTol = 6;
  
  m_useNmScale  = false;
  m_nmScale     = 1.0;
#ifdef SCALE_FILE
  m_nmScaleFile = SCALE_FILE;
#else
  m_nmScaleFile = "scale.txt";
#endif

  // Used for undo
  m_polyVecStack.clear();
  m_polyOptionsVecStack.clear();
  m_highlightsStack.clear();
  m_resetViewStack.clear();
  m_posInUndoStack = -1;

  // Show poly diff mode
  m_diffColorsMode = false;
  m_polyDiffMode   = false;
  m_polyVecBk.clear();
  m_polyOptionsVecBk.clear();
  m_distVec.clear(); // distances b/w polys to diff
  m_indexOfDistToPlot = -1;

  // Plot the point wheres the mouse clicked (with or without snapping
  // to the closest vertex).
  m_snappedPoints.clear();
  m_nonSnappedPoints.clear();
  m_smallLen = 2; // Used for plotting the points

  // Edit mode
  m_moveVertices            = false;
  m_moveEdges               = false;
  m_movePolys               = true;
  m_toggleShowPointsEdgesBk = m_showEdges;
  m_polyVecIndex            = -1;
  m_polyIndexInCurrPoly     = -1;
  m_vertIndexInCurrPoly     = -1;
  
  // Align mode
  m_alignMode = false;
  
#ifdef ADD_PREF_INI
  readINI();
//  m_showAnnotations    = utils::loadSettingBool("m_showAnnotations",m_showAnnotations);
//  m_showVertIndexAnno  = utils::loadSettingBool("m_showVertIndexAnno",m_showVertIndexAnno);
//  m_showLayerAnno      = utils::loadSettingBool("m_showLayerAnno",m_showLayerAnno);
#endif // ADD_PREF_INI

  resetTransformSettings();

  // This statement must be towards the end
  readAllPolys(); // To do: avoid global variables here

  return;
}

bool polyView::eventFilter(QObject *obj, QEvent *E){


  QHoverEvent * H = dynamic_cast<QHoverEvent*>(E);
  
  if (H && m_createPoly){
    
    // The mouse is hovering and we are creating a new poly. Update
    // the screen and then draw on top of it the polygon being
    // plotted, with its last vertex connected to the current mouse
    // pointer.

    const QPoint Q = H->pos(); // mouse point position
    int x = Q.x();
    int y = Q.y() - 25; // A hack as Qt does not give this number correctly
    double wx, wy;
    pixelToWorldCoords(x, y, wx, wy);

    // We are forced to do things via the "bk" hack since we cannot do
    // a paint event from the current function, so need to call
    // refreshPixmap() to repaint the screen.
    vector<double> polyX_bk = m_currPolyX;
    vector<double> polyY_bk = m_currPolyY;
    m_currPolyX.push_back(wx);
    m_currPolyY.push_back(wy);
    refreshPixmap();
    m_currPolyX = polyX_bk;
    m_currPolyY = polyY_bk;
  }
  
  return QWidget::eventFilter(obj, E);
}

void polyView::setupViewingWindow(){

  // Dimensions of the plotting window in pixels exluding any window
  // frame/menu bar/status bar
  QRect v       = this->geometry();
  m_screenXll   = v.left();
  m_screenYll   = v.top();
  m_screenWidX  = v.width();
  m_screenWidY  = v.height();
  //cout << "geom is: " << m_screenXll << ' ' << m_screenYll << ' '
  //     << m_screenWidX << ' ' << m_screenWidY << endl;
  
  if (m_resetView){
    setUpViewBox(// inputs
                 m_polyVec,
                 // outputs
                 m_viewXll, m_viewYll, m_viewWidX, m_viewWidY
                 );
    m_resetView = false;
  }

  //  This is necessary when the screen is resized
  m_screenRatio = double(m_screenWidY)/double(m_screenWidX);
  expandBoxToGivenRatio(// Inputs
                        m_screenRatio,
                        // Inputs-outputs
                        m_viewXll, m_viewYll, m_viewWidX, m_viewWidY
                        );

  // Create the new view
  double xll, yll, xur, yur, widx, widy;
    
  if (m_zoomToMouseSelection){
    
    // Form a new view based on the rectangle selected with the mouse.
    // The call to pixelToWorldCoords uses the existing view internally.
    pixelToWorldCoords(m_mousePrsX, m_mousePrsY, xll, yur); // upper-left  rect corner
    pixelToWorldCoords(m_mouseRelX, m_mouseRelY, xur, yll); // lower-right rect corner
    widx = xur - xll;
    widy = yur - yll;

  }else if (m_viewChanged){

    // Modify the view for given shift or zoom
    xll  = m_viewXll + m_viewWidX*( (1 - m_zoomFactor)/2.0 + m_shiftX );
    yll  = m_viewYll + m_viewWidY*( (1 - m_zoomFactor)/2.0 + m_shiftY );
    widx = m_viewWidX*m_zoomFactor;
    widy = m_viewWidY*m_zoomFactor;
    
    resetTransformSettings(); // Wipe the zoom and shift data 
  
  }

  if (m_zoomToMouseSelection || m_viewChanged){
    
    // If the view becomes too small, don't accept it
    if (xll + widx <= xll || yll + widy <= yll){
      cerr << "Cannot zoom to requested view."  << endl;
    }else{
      // Enlarge this rectangle if necessary to keep the aspect ratio.
      expandBoxToGivenRatio(//inputs
                            m_screenRatio,  
                            // input/outputs
                            xll, yll, widx, widy
                            );
      
      // Overwrite the view
      m_viewXll = xll; m_viewWidX = widx;
      m_viewYll = yll; m_viewWidY = widy;
    }
    printCmd("view", m_viewXll, m_viewYll, m_viewWidX, m_viewWidY);
    m_zoomToMouseSelection = false;
    m_viewChanged          = false;
  }

  // The two ratios below will always be the same. Take the maximum
  // for robustness to floating point errors.
  m_pixelSize = max(m_viewWidX/m_screenWidX, m_viewWidY/m_screenWidY);

  return;
}

void polyView::displayData( QPainter *paint ){

  setupViewingWindow(); // Must happen before anything else
  
  // This vector is used for sparsing out text on screen
  vector< vector<int> > textOnScreenGrid; 

  // Build the grid if the user wants to
  if (m_prefs.isGridOn && m_prefs.gridWidth > 0){

    if (m_prefs.gridSize <= 0)
      m_prefs.gridSize = calcGrid(m_viewWidX, m_viewWidY);
    
    dPoly grid;
    bool plotPoints = false, plotEdges = true, plotFilled = false;
    bool showAnno = false;
    int drawVertIndex = 0;
    textOnScreenGrid.clear(); // (this is text grid, not line grid)
    grid.buildGrid(m_viewXll,  m_viewYll,
                   m_viewXll + m_viewWidX,
                   m_viewYll + m_viewWidY,
                   m_prefs.gridSize, m_prefs.gridColor
                   );
    plotDPoly(plotPoints, plotEdges, plotFilled, showAnno, m_prefs.gridWidth, 
              drawVertIndex, textOnScreenGrid, paint, grid
              );
  }
  
  
  // Plot the polygons
  setupDisplayOrder(m_polyVec.size(),                    //inputs
                    m_changeDisplayOrder, m_polyVecOrder // inputs-outputs
                    );
  // Will draw a vertex with a shape dependent on this index
  int drawVertIndex = -1; 
  // Use a grid to not draw text too densely as that's slow
  initTextOnScreenGrid(textOnScreenGrid);
  assert( m_polyVec.size() == m_polyOptionsVec.size() );
  for (int vi  = 0; vi < (int)m_polyVec.size(); vi++){

    int vecIter = m_polyVecOrder[vi];

    // Skip the files the user does not want to see
    string fileName = m_polyOptionsVec[vecIter].polyFileName;
    if (m_filesToHide.find(fileName) != m_filesToHide.end()) continue;
      
    int lineWidth = m_polyOptionsVec[vecIter].lineWidth;

    // Note: plotFilled, plotEdges, and plotPoints are not mutually exclusive.
    bool plotFilled = m_polyOptionsVec[vecIter].isPolyFilled || m_showFilledPolys;
    bool plotEdges  = (!m_polyOptionsVec[vecIter].plotAsPoints) &&
      (m_toggleShowPointsEdges != m_showPoints);
    bool plotPoints = m_polyOptionsVec[vecIter].plotAsPoints  ||
      ( m_toggleShowPointsEdges == m_showPoints )             ||
      ( m_toggleShowPointsEdges == m_showPointsEdges);
    
    if (plotPoints) drawVertIndex++;

    bool showAnno = true;
    plotDPoly(plotPoints, plotEdges, plotFilled, showAnno, lineWidth, 
              drawVertIndex, textOnScreenGrid, paint, m_polyVec[vecIter]
              );
    if ( !plotFilled && !m_selectedPolyIndices[vecIter].empty() ){
      // Plot the selected polys on top with thicker lines
      dPoly lPolys;
      int lineWidth2 = 2*lineWidth;
      m_polyVec[vecIter].extractMarkedPolys(m_selectedPolyIndices[vecIter], // input
                                            lPolys                          // output
                                            );
      plotDPoly(plotPoints, plotEdges, plotFilled, showAnno, lineWidth2, 
                drawVertIndex, textOnScreenGrid, paint, lPolys
                );
    }
    
  } // End iterating over sets of polygons

  // Plot the highlights
  bool plotPoints = false, plotEdges = true, plotFilled = false;
  drawVertIndex = 0;
  textOnScreenGrid.clear();
  for (int h = 0; h < (int)m_highlights.size(); h++){
    m_highlights[h].set_color(m_prefs.fgColor.c_str());
    bool showAnno = false;
    plotDPoly(plotPoints, plotEdges, plotFilled, showAnno, m_prefs.lineWidth, 
              drawVertIndex, textOnScreenGrid, paint, m_highlights[h]
              );
  }
  
  // This draws the polygon being created if in that mode
  drawPolyBeingPlotted(m_currPolyX, m_currPolyY, paint);

  // Draw the mark if there
  if (m_markX.size() > 0){
    int x0, y0;
    worldToPixelCoords(m_markX[0], m_markY[0], // inputs
                       x0, y0                  // outputs
                       );
    drawMark(x0, y0, QColor(m_prefs.fgColor.c_str()),
             m_prefs.lineWidth, paint);
  }

  // If in diff mode
  plotDistBwPolyClips(paint);

  // Wipe this temporary data now that the display changed
  m_snappedPoints.clear();
  m_nonSnappedPoints.clear(); 
  
  return;
}

void polyView::plotDPoly(bool plotPoints, bool plotEdges,
                         bool plotFilled, bool showAnno,
                         int lineWidth,
                         int drawVertIndex, // 0 is a good choice here
                         // An empty grid is a good choice if not text is present
                         std::vector< std::vector<int> > & textOnScreenGrid,
                         QPainter *paint,
                         dPoly currPoly // Make a local copy on purpose
                         ){

  // Plot a given dPoly with given options.
  
  // Note: Having annotations at vertices can make the display
  // slow for large polygons.
  // The operations below must happen before cutting,
  // as cutting will inherit the result computed here.
  if (m_showVertIndexAnno){
    currPoly.compVertIndexAnno();
  }else if (m_showLayerAnno){
    currPoly.compLayerAnno();
  }
    
  // When polys are filled, plot largest polys first
  if (plotFilled)
    currPoly.sortBySizeAndMaybeAddBigContainingRect(m_viewXll,  m_viewYll,
                                                    m_viewXll + m_viewWidX,
                                                    m_viewYll + m_viewWidY
                                                    );

  // Clip the polygon a bit beyond the viewing window, as to not see
  // the edges where the cut took place. It is a bit tricky to
  // decide how much the extra should be.
  double tol    = 1e-12;
  double extra  = 2*m_pixelSize*lineWidth;
  double extraX = extra + tol*max(abs(m_viewXll), abs(m_viewXll + m_viewWidX));
  double extraY = extra + tol*max(abs(m_viewYll), abs(m_viewYll + m_viewWidY));
  
  dPoly clippedPoly;
  currPoly.clipPoly(//inputs
                    m_viewXll - extraX,
                    m_viewYll - extraY,
                    m_viewXll + m_viewWidX + extraX,
                    m_viewYll + m_viewWidY + extraY,
                    // output
                    clippedPoly
                    );

  const double * xv               = clippedPoly.get_xv();
  const double * yv               = clippedPoly.get_yv();
  const int    * numVerts         = clippedPoly.get_numVerts();
  int numPolys                    = clippedPoly.get_numPolys();
  const vector<char> isPolyClosed = clippedPoly.get_isPolyClosed();
  const vector<string> colors     = clippedPoly.get_colors();
  //int numVerts                  = clippedPoly.get_totalNumVerts();
    
  vector<anno> annotations;
  annotations.clear();
  if (showAnno){
    if (m_showVertIndexAnno){
      clippedPoly.get_vertIndexAnno(annotations);
    }else if (m_showLayerAnno){
      clippedPoly.get_layerAnno(annotations);
    }else if (m_showAnnotations){
      clippedPoly.get_annotations(annotations);
    }
  }
  
  int start = 0;
  for (int pIter = 0; pIter < numPolys; pIter++){

    if (pIter > 0) start += numVerts[pIter - 1];

    // Change the poly file color if it is the background color or invalid
    QColor color = QColor( colors[pIter].c_str() );
#ifdef USE_QT4_DEFS
    if ( color == backgroundColor() || color == QColor::Invalid){
      if ( backgroundColor() != QColor("white") ){
        color = QColor("white");
      }else{
        color = QColor("black");
      }
    }
#else
    // #ifdef _MSC_VER
    // #pragma message("TODO: DONE: set backglound color... I think...")
    // #endif
    QColor bkgnd = palette().color(QWidget::backgroundRole());
    //QColor bkgnd = QWidget::palette().color(QWidget::backgroundRole());
    if ( color == bkgnd || color == QColor::Invalid){
      if ( bkgnd != QColor("white") ){
        color = QColor("white");
      }else{
        color = QColor("black");
      }
    }
#endif

    int pSize = numVerts[pIter];

    // Determine the orientation of polygons
    double signedArea = 0.0;
    if (plotFilled && isPolyClosed[pIter]){
      signedArea = signedPolyArea(pSize, xv + start, yv + start);
    }
      
#ifdef USE_QT4_DEFS // Q3PointArray
    Q3PointArray pa(pSize);
#else 
    QPolygon pa(pSize);
#endif
    for (int vIter = 0; vIter < pSize; vIter++){

      int x0, y0;
      worldToPixelCoords(xv[start + vIter], yv[start + vIter], // inputs
                         x0, y0                                // outputs
                         );
      pa[vIter] = QPoint(x0, y0);

      // Qt's built in points are too small. Instead of drawing a point
      // draw a small shape.
      int tol = 4; // This is a bug fix for missing points. I don't understand
      //           // why this is necessary and why the number 4 is right.  
      if ( plotPoints                                                  &&
           x0 > m_screenXll - tol && x0 < m_screenXll + m_screenWidX + tol && 
           y0 > m_screenYll - tol && y0 < m_screenYll + m_screenWidY + tol
           ){
        drawOneVertex(x0, y0, color, lineWidth, drawVertIndex, paint);
      }
    }
      
    if (plotEdges){

      if (plotFilled && isPolyClosed[pIter]) {
          if (signedArea >= 0.0) {
              paint->setBrush( color );
          } else {
#ifdef USE_QT4_DEFS // backgroundColor()
              paint->setBrush( backgroundColor() );
#else
    // #ifdef _MSC_VER
    // #pragma message( "TODO: DONE: Get the backgroundColor()... I think" )
    // #endif
              paint->setBrush( bkgnd ); // backgroundColor() );
#endif
          }
          paint->setPen( Qt::NoPen );
      }else {
        paint->setBrush( Qt::NoBrush );
        paint->setPen( QPen(color, lineWidth) );
      }

      if ( pa.size() >= 1 && isPolyZeroDim(pa) ){
        // Treat the case of polygons which are made up of just one point
        int l_drawVertIndex = -1;
        drawOneVertex(pa[0].x(), pa[0].y(), color, lineWidth, l_drawVertIndex,
                      paint);
      }else if (isPolyClosed[pIter]){
        paint->drawPolygon( pa );
      }else{
        paint->drawPolyline( pa ); // don't join the last vertex to the first
      }
        
    }
  }

    // Plot and PAINT the annotations
    // TODO: Note 'fixed' annotation color!!!
    // TODO: FAILS to correctly display UTF-8 hi-bit chars - maybe QString::fromUtf8???
    int numAnno = annotations.size();
    for (int aIter = 0; aIter < numAnno; aIter++) {
        const anno & A = annotations[aIter];
        int x0, y0;
        worldToPixelCoords(A.x, A.y, // inputs
                       x0, y0    // outputs
                       );
        paint->setPen( QPen(QColor("gold"), lineWidth) );
        if (isClosestGridPtFree(textOnScreenGrid, x0, y0)) {
            paint->drawText(x0, y0, (A.annoLabel).c_str());
            // maybe????
            //QString s = QString::fromUtf8(A.annoLabel.c_str());
            //paint->drawText(x0, y0, s);
        }
      
    } // End placing annotations
  
    return;
}

void polyView::zoomIn(){
  m_zoomFactor  = 0.5;
  m_viewChanged = true;
  refreshPixmap();
}

void polyView::zoomOut(){
  m_zoomFactor  = 2.0;
  m_viewChanged = true;
  refreshPixmap();
}

void polyView::shiftRight(){
  m_shiftX      = 0.25;
  m_viewChanged = true;
  refreshPixmap();
}

void polyView::shiftLeft(){
  m_shiftX      = -0.25;
  m_viewChanged = true;
  refreshPixmap();
}

void polyView::shiftUp(){
  m_shiftY      = 0.25;
  m_viewChanged = true;
  refreshPixmap();
}

void polyView::shiftDown(){
  m_shiftY      = -0.25;
  m_viewChanged = true;
  refreshPixmap();
}

void polyView::centerViewAtPoint(double x, double y){
  m_viewXll     = x - m_viewWidX/2.0;
  m_viewYll     = y - m_viewWidY/2.0;
  m_viewChanged = true;
}

void polyView::resetView(){
  m_resetView   = true;
  m_viewChanged = true;
  refreshPixmap();
}


void polyView::resetTransformSettings(){
  m_zoomFactor = 1.0;
  m_shiftX     = 0.0; m_shiftY = 0.0;
}

void polyView::mousePressEvent( QMouseEvent *E){

  const QPoint Q = E->pos();
  m_mousePrsX = Q.x();
  m_mousePrsY = Q.y();
  // GRAB ALL ON PRESS EVENT
  m_mouseMods = E->modifiers();
  m_mouseButt = E->button();
  m_mouseButs = E->buttons();

#if 0
  cout << "Mouse pressed at "
       << m_mousePrsX << ' ' << m_mousePrsY << endl;
#endif

  pixelToWorldCoords(m_mousePrsX, m_mousePrsY,              // inputs
                     m_mousePressWorldX, m_mousePressWorldY // outputs
                     ); 

  m_rubberBand = m_emptyRubberBand;

  // This must happen before m_movingVertsOrEdgesOrPolysNow is declared.
  m_deletingPolyNow = ( ( E->modifiers() & Qt::AltModifier   ) &&
                        ( E->modifiers() & Qt::ShiftModifier )
                        );

  m_aligningPolysNow = ( m_alignMode && isShiftLeftMouse(E) && !m_createPoly );
  if (m_aligningPolysNow){
    assert(m_polyVec.size() >= 1);
    m_polyBeforeShift = m_polyVec[0];
    m_T.reset();
  }
  
  m_movingVertsOrEdgesOrPolysNow = ( (m_moveVertices || m_moveEdges || m_movePolys) &&
                                     isShiftLeftMouse(E)                            &&
                                     !m_createPoly && !m_deletingPolyNow
                                     );
  m_movingPolysInHlts = false;
  if (m_movingVertsOrEdgesOrPolysNow){

    double min_x, min_y, min_dist;
    if (m_moveVertices){
      findClosestPolyVertex(// inputs
                            m_mousePressWorldX, m_mousePressWorldY, m_polyVec,
                            // outputs
                            m_polyVecIndex,
                            m_polyIndexInCurrPoly,
                            m_vertIndexInCurrPoly,
                            min_x, min_y, min_dist
                            );
    }else if (m_movePolys && getNumElements(m_selectedPolyIndices) > 0 ){
      m_highlights.clear(); // No need for these anymore
      m_polyVecBeforeShift = m_polyVec;
      m_movingPolysInHlts = true;
    }else if (m_moveEdges || m_movePolys ){
      findClosestPolyEdge(// inputs
                          m_mousePressWorldX, m_mousePressWorldY, m_polyVec,  
                          // outputs
                          m_polyVecIndex,
                          m_polyIndexInCurrPoly,
                          m_vertIndexInCurrPoly,
                          min_x, min_y, min_dist
                          );
      if (m_polyVecIndex >= 0) m_polyBeforeShift = m_polyVec[m_polyVecIndex];
    }
    
    return;
  }

  return;
}

void polyView::mouseMoveEvent( QMouseEvent *E){

  QPoint Q = E->pos();
  int x = Q.x(), y = Q.y();

  double wx, wy;
  pixelToWorldCoords(x, y, wx, wy);

  double shift_x = wx - m_mousePressWorldX;
  double shift_y = wy - m_mousePressWorldY;
  
  if (m_aligningPolysNow){
    m_polyVec[0] = m_polyBeforeShift;
    m_polyVec[0].applyTransform(1, 0, 0, 1, shift_x, shift_y, m_T);
    refreshPixmap();
    return;
  }

  if (m_movingPolysInHlts){
    m_polyVec = m_polyVecBeforeShift;
    shiftMarkedPolys(// Inputs
                     m_selectedPolyIndices, shift_x, shift_y,
                     // Inputs-outputs
                     m_polyVec
                     );
    refreshPixmap();
    return;
  }

  if (m_movingVertsOrEdgesOrPolysNow){

    if (m_polyVecIndex        < 0 ||
        m_polyIndexInCurrPoly < 0 ||
        m_vertIndexInCurrPoly < 0) return;
    
    if (m_moveVertices){
      m_polyVec[m_polyVecIndex].changeVertexValue(m_polyIndexInCurrPoly,
                                                  m_vertIndexInCurrPoly,
                                                  wx, wy
                                                  );
    }else if ( (m_moveEdges || m_movePolys) && m_polyVecIndex >= 0){
      m_polyVec[m_polyVecIndex] = m_polyBeforeShift;
      if (m_moveEdges){
        m_polyVec[m_polyVecIndex].shiftEdge(m_polyIndexInCurrPoly,
                                            m_vertIndexInCurrPoly,
                                            shift_x, shift_y 
                                            );
      }else if (m_movePolys){
        m_polyVec[m_polyVecIndex].shiftOnePoly(m_polyIndexInCurrPoly,
                                               shift_x, shift_y 
                                               );
      }
    }
    refreshPixmap(); // To do: Need to update just a small region, not the whole screen
    return;
  }

  // Standard Qt rubberband trick (kind of confusing as to how it works).
  updateRubberBand(m_rubberBand);
  m_rubberBand = QRect( min(m_mousePrsX, x), min(m_mousePrsY, y),
                        abs(x - m_mousePrsX), abs(y - m_mousePrsY) );
  updateRubberBand(m_rubberBand);
  
  return;
}

void polyView::mouseReleaseEvent ( QMouseEvent * E ){

  const QPoint Q = E->pos();
  m_mouseRelX = Q.x();
  m_mouseRelY = Q.y();
#if 0
  cout << "Mouse pressed at "
       << m_mousePrsX << ' ' << m_mousePrsY << endl;
  cout << "Mouse released at "
       << m_mouseRelX << ' ' << m_mouseRelY << endl;
#endif
  // Qt::ShiftModifier, Qt::ControlModifier, Qt::AltModifier
  Qt::KeyboardModifiers mods = E->modifiers();
  // Qt::LeftButton, Qt::RightButton, Qt::MidButton 
  Qt::MouseButtons buts = E->buttons();

  pixelToWorldCoords(m_mouseRelX, m_mouseRelY, m_menuX, m_menuY);
  
  if ( m_deletingPolyNow ){
    // To do: consolidate this with the other call to this function.
    // See if can pass the relevant variables as input arguments.
    deletePoly();
    return;
  }

  if (m_aligningPolysNow){
    m_T.print();
    m_totalT = composeTransforms(m_T, m_totalT);
  }
  
  if (m_aligningPolysNow || m_movingVertsOrEdgesOrPolysNow){
    saveDataForUndo(false);
    refreshPixmap();
    return;
  }

  // Wipe the rubberband
  updateRubberBand(m_rubberBand); 
  m_rubberBand = m_emptyRubberBand;
  updateRubberBand(m_rubberBand);

  if ( E->modifiers() & Qt::ControlModifier ){
    // Draw a  highlight with control + left mouse button
    // ending at the current point
    createHighlightWithPixelInputs(m_mousePrsX, m_mousePrsY, m_mouseRelX, m_mouseRelY);
    refreshPixmap();
    return;
  }

  int tol = 10; 
  // Any selection smaller than 'tol' number of pixels will be ignored
  // as perhaps the user moved the mouse unintentionally between press
  // and release.
  if       (m_mouseRelX > m_mousePrsX + tol &&
            m_mouseRelY > m_mousePrsY + tol){
    
    m_zoomToMouseSelection = true; // Will zoom to the region selected with the mouse
    refreshPixmap(); 
    return;
    
  }else if (m_mouseRelX + tol < m_mousePrsX &&
            m_mouseRelY + tol < m_mousePrsY ){
    
    zoomOut();
    return;
    
  }else if (abs(m_mouseRelX - m_mousePrsX) <= tol &&
            abs(m_mouseRelY - m_mousePrsY) <= tol){

  
    if (m_createPoly){
      addPolyVert(m_mouseRelX, m_mouseRelY);
      refreshPixmap();

      return;
    }

#ifdef USE_QT4_DEFS
    printCurrCoords(E->state(),              // input
                    m_mouseRelX, m_mouseRelY // in-out
                    );
#else
//#ifdef _MSC_VER
//#pragma message( "*** TODO: DONE: print the cooordinates" )
//#endif
    
    printCurrCoords(E->modifiers(),              // input
                    m_mouseRelX, m_mouseRelY // in-out
                    );
#endif // QT4 y/n

    return;
  }
  
  return;
}

bool polyView::isShiftLeftMouse(QMouseEvent * E){
  // This does not work in mouseReleaseEvent. 
  return ( E->buttons() & Qt::LeftButton  ) && ( E->modifiers() & Qt::ShiftModifier );
}

void polyView::wheelEvent(QWheelEvent *E){

  int delta = E->delta();
#ifdef USE_QT4_DEFS // QWheelEvent - detect control down
  int gotctl = (E->state() == Qt::ControlModifier ? 1 : 0);
#else
//#ifdef _MSC_VER
//#pragma message( "*** TODO: DONE: check for control and shift keys - seems ok" )
//#endif
  int gotctl = (E->modifiers() & Qt::ControlModifier ? 1 : 0);
#endif
  if (gotctl) {

    // The control button was pressed. Zoom in/out around the current point.

    int pixelx = E->x();
    int pixely = E->y();
    double x, y;
    pixelToWorldCoords(pixelx, pixely, x, y);
    centerViewAtPoint(x, y);
    
    if (delta > 0){
      zoomIn();
    }else if (delta < 0){
      zoomOut();
    }
    
  } else {
      // Shift wheel goes left and right. Without shift we go up and down.
#ifdef USE_QT4_DEFS // fix E->state() == Qt::ShiftModifier
      int gotshift = (E->state() == Qt::ShiftModifier) ? 1 : 0;
#else 
      int gotshift = (E->modifiers() & Qt::ShiftModifier ? 1 : 0);
#endif
    if (gotshift) {
      if (delta > 0){
        shiftLeft();
      }else if (delta < 0){
        shiftRight();
      }
    }else{
      if (delta > 0){
        shiftUp();
      }else if (delta < 0){
        shiftDown();
      }
    }
  }
  
  E->accept();
}

void polyView::keyPressEvent( QKeyEvent *K ){

  switch ( K->key() ) {
  case Qt::Key_Minus:
    zoomOut();
    break;
  case Qt::Key_Plus:
    zoomIn();
    break;
  case Qt::Key_Equal:
    zoomIn();
    break;
  case Qt::Key_Right:
    shiftRight();
    break;
  case Qt::Key_Left:
    shiftLeft();
    break;
  case Qt::Key_Up:
    shiftUp();
    break;
  case Qt::Key_Down:
    shiftDown();
    break;
  }
  
}

void polyView::contextMenuEvent(QContextMenuEvent *E){

  int x = E->x();
  int y = E->y();

  pixelToWorldCoords(x, y, m_menuX, m_menuY);

#ifdef USE_QT4_DEFS
  Q3PopupMenu menu(this);
  int id = 1;

  menu.insertItem("Save mark at point", this, SLOT(saveMark()));
  
  menu.insertItem("Use nm scale", this, SLOT(toggleNmScale()), 0, id);
  menu.setItemChecked(id, m_useNmScale);
  id++;
  
  menu.insertItem("Create 45-degree integer polygon", this,
                  SLOT(create45DegreeIntPoly()));
  menu.insertItem("Create arbitrary polygon", this,
                  SLOT(createArbitraryPoly()));
  menu.insertItem("Delete polygon (Alt-Shift-Mouse)", this, SLOT(deletePoly()));
  menu.addSeparator();

  menu.insertItem("Align mode", this, SLOT(toggleAlignMode()), 0, id);
  menu.setItemChecked(id, m_alignMode);
  id++;
    
  if (m_alignMode){
    menu.insertItem("Rotate  90 degrees",  this, SLOT(align_rotate90()));
    menu.insertItem("Rotate 180 degrees",  this, SLOT(align_rotate180()));
    menu.insertItem("Rotate 270 degrees",  this, SLOT(align_rotate270()));
    menu.insertItem("Flip against x axis", this, SLOT(align_flip_against_x_axis()));
    menu.insertItem("Flip against y axis", this, SLOT(align_flip_against_y_axis()));
    menu.insertItem("Guess alignment", this,
                    SLOT(performAlignmentOfClosePolys()));
  }
  
  menu.addSeparator();

  menu.insertItem("Move polygons (Shift-Mouse)", this,
                  SLOT(turnOnMovePolys()), 0, id);
  menu.setItemChecked(id, m_movePolys);
  id++;

  menu.insertItem("Move vertices (Shift-Mouse)", this,
                  SLOT(turnOnMoveVertices()), 0, id);
  menu.setItemChecked(id, m_moveVertices);
  id++;
  
  menu.insertItem("Move edges (Shift-Mouse)", this,
                  SLOT(turnOnMoveEdges()), 0, id);
  menu.setItemChecked(id, m_moveEdges);
  id++;
  
  menu.insertItem("Insert vertex on edge", this, SLOT(insertVertex()));
  menu.insertItem("Delete vertex",         this, SLOT(deleteVertex()));
  menu.insertItem("Copy polygon",          this, SLOT(copyPoly()));
  menu.insertItem("Paste polygon",         this, SLOT(pastePoly()));
  menu.insertItem("Reverse orientation",   this, SLOT(reversePoly()));
      
  menu.exec(E->globalPos());

#else
    // Q3PopupMenu menu(this);
    QMenu menu(this);
    int id = 1;
// #### GEN CODE ############################################## 
// gen Qt5 code, gen by qt42qt5.pl, on 2016/06/23 15:19:48 UTC.
  saveMarkAct = new QAction(tr("Save mark at point"),this);
  saveMarkAct->setStatusTip(tr("Save mark at point"));
  connect(saveMarkAct, SIGNAL(triggered()), this, SLOT(saveMark()));

  toggleNmScaleAct = new QAction(tr("Toggle nm scale"),this);
  toggleNmScaleAct->setStatusTip(tr("Toggle nm scale"));
  //toggleNmScaleAct->setShortcut(0);
  // menu.setItemChecked(id, m_useNmScale);
  toggleNmScaleAct->setChecked(!m_useNmScale);
  id++;
  connect(toggleNmScaleAct, SIGNAL(triggered()), this, SLOT(toggleNmScale()));

  create45DegreeIntPolyAct = new QAction(tr("Create 45-degree integer polygon"),this);
  create45DegreeIntPolyAct->setStatusTip(tr("Create 45-degree integer polygon"));
  connect(create45DegreeIntPolyAct, SIGNAL(triggered()), this, SLOT(create45DegreeIntPoly()));

  createArbitraryPolyAct = new QAction(tr("Create arbitrary polygon"),this);
  createArbitraryPolyAct->setStatusTip(tr("Create arbitrary polygon"));
  connect(createArbitraryPolyAct, SIGNAL(triggered()), this, SLOT(createArbitraryPoly()));

  deletePolyAct = new QAction(tr("Delete polygon (Alt-Shift-Mouse)"),this);
  deletePolyAct->setStatusTip(tr("Delete polygon (Alt-Shift-Mouse)"));
  connect(deletePolyAct, SIGNAL(triggered()), this, SLOT(deletePoly()));

  // toggleAlignModeAct = new QAction(tr("Toggle Align mode"),this);
  if (m_polyVec.size() < 2) {
    toggleAlignModeAct = new QAction(tr("No Align mode toggle"),this);
  } else {
      if (m_alignMode) {
        toggleAlignModeAct = new QAction(tr("Align mode to OFF"),this);
      } else {
        toggleAlignModeAct = new QAction(tr("Align mode to ON"),this);
      }
  }
  toggleAlignModeAct->setStatusTip(tr("Toggle Align mode"));
  //toggleAlignModeAct->setShortcut(0);
  // menu.setItemChecked(id, m_alignMode);
  toggleAlignModeAct->setChecked(m_alignMode);
  id++;
  connect(toggleAlignModeAct, SIGNAL(triggered()), this, SLOT(toggleAlignMode()));

  // Only added if m_alignMode is ON
  ////////////////////////////////////////////
  align_rotate90Act = new QAction(tr("Rotate 90 degrees"),this);
  align_rotate90Act->setStatusTip(tr("Rotate 90 degrees"));
  connect(align_rotate90Act, SIGNAL(triggered()), this, SLOT(align_rotate90()));

  align_rotate180Act = new QAction(tr("Rotate 180 degrees"),this);
  align_rotate180Act->setStatusTip(tr("Rotate 180 degrees"));
  connect(align_rotate180Act, SIGNAL(triggered()), this, SLOT(align_rotate180()));

  align_rotate270Act = new QAction(tr("Rotate 270 degrees"),this);
  align_rotate270Act->setStatusTip(tr("Rotate 270 degrees"));
  connect(align_rotate270Act, SIGNAL(triggered()), this, SLOT(align_rotate270()));

  align_flip_against_x_axisAct = new QAction(tr("Flip against x axis"),this);
  align_flip_against_x_axisAct->setStatusTip(tr("Flip against x axis"));
  connect(align_flip_against_x_axisAct, SIGNAL(triggered()), this, SLOT(align_flip_against_x_axis()));

  align_flip_against_y_axisAct = new QAction(tr("Flip against y axis"),this);
  align_flip_against_y_axisAct->setStatusTip(tr("Flip against y axis"));
  connect(align_flip_against_y_axisAct, SIGNAL(triggered()), this, SLOT(align_flip_against_y_axis()));

  performAlignmentOfClosePolysAct = new QAction(tr("Guess alignment"),this);
  performAlignmentOfClosePolysAct->setStatusTip(tr("Guess alignment"));
  connect(performAlignmentOfClosePolysAct, SIGNAL(triggered()), this, SLOT(performAlignmentOfClosePolys()));
  ////////////////////////////////////////////

  turnOnMovePolysAct = new QAction(tr("Move polygons (Shift-Mouse)"),this);
  turnOnMovePolysAct->setStatusTip(tr("Move polygons (Shift-Mouse)"));
  //turnOnMovePolysAct->setShortcut(0);
  connect(turnOnMovePolysAct, SIGNAL(triggered()), this, SLOT(turnOnMovePolys()));

  turnOnMoveVerticesAct = new QAction(tr("Move vertices (Shift-Mouse)"),this);
  turnOnMoveVerticesAct->setStatusTip(tr("Move vertices (Shift-Mouse)"));
  //turnOnMoveVerticesAct->setShortcut(0);
  connect(turnOnMoveVerticesAct, SIGNAL(triggered()), this, SLOT(turnOnMoveVertices()));

  turnOnMoveEdgesAct = new QAction(tr("Move edges (Shift-Mouse)"),this);
  turnOnMoveEdgesAct->setStatusTip(tr("Move edges (Shift-Mouse)"));
  //turnOnMoveEdgesAct->setShortcut(0);
  connect(turnOnMoveEdgesAct, SIGNAL(triggered()), this, SLOT(turnOnMoveEdges()));

  insertVertexAct = new QAction(tr("Insert vertex on edge"),this);
  insertVertexAct->setStatusTip(tr("Insert vertex on edge"));
  connect(insertVertexAct, SIGNAL(triggered()), this, SLOT(insertVertex()));

  deleteVertexAct = new QAction(tr("Delete vertex"),this);
  deleteVertexAct->setStatusTip(tr("Delete vertex"));
  connect(deleteVertexAct, SIGNAL(triggered()), this, SLOT(deleteVertex()));

  copyPolyAct = new QAction(tr("Copy polygon"),this);
  copyPolyAct->setStatusTip(tr("Copy polygon"));
  connect(copyPolyAct, SIGNAL(triggered()), this, SLOT(copyPoly()));

  pastePolyAct = new QAction(tr("Paste polygon"),this);
  pastePolyAct->setStatusTip(tr("Paste polygon"));
  connect(pastePolyAct, SIGNAL(triggered()), this, SLOT(pastePoly()));

  reversePolyAct = new QAction(tr("Reverse orientation"),this);
  reversePolyAct->setStatusTip(tr("Reverse orientation"));
  connect(reversePolyAct, SIGNAL(triggered()), this, SLOT(reversePoly()));

  pvExitAct = new QAction(tr("Exit"),this);
  connect(pvExitAct, SIGNAL(triggered()), m_parent, SLOT(forceQuit()));

// add 20 actions to menu menuMenu
  menu.addAction(saveMarkAct);
  menu.addAction(toggleNmScaleAct);
  menu.addAction(create45DegreeIntPolyAct);
  menu.addAction(createArbitraryPolyAct);
  menu.addAction(deletePolyAct);
  menu.addSeparator();
  menu.addAction(toggleAlignModeAct);
  if (m_alignMode) {
      menu.addAction(align_rotate90Act);
      menu.addAction(align_rotate180Act);
      menu.addAction(align_rotate270Act);
      menu.addAction(align_flip_against_x_axisAct);
      menu.addAction(align_flip_against_y_axisAct);
      menu.addAction(performAlignmentOfClosePolysAct);
  }
  menu.addSeparator();
  menu.addAction(turnOnMovePolysAct);
  menu.addAction(turnOnMoveVerticesAct);
  menu.addAction(turnOnMoveEdgesAct);
  menu.addAction(insertVertexAct);
  menu.addAction(deleteVertexAct);
  menu.addAction(copyPolyAct);
  menu.addAction(pastePolyAct);
  menu.addAction(reversePolyAct);

// #### END GEN CODE ############################################## 
  menu.addSeparator();
  menu.addAction(pvExitAct);

#if 0 // 000000000000000000000000000000000000000000000000000000000000000
    // menu.insertItem("Save mark at point", this, SLOT(saveMark()));
    QAction *saveMarkAct = new QAction(tr("Save mark at point"),this);
    saveMarkAct->setStatusTip(tr("Save mark at point"));
    //saveMarkAct->setShortcut(Qt::CTRL+Qt::Key_O);
    connect(saveMarkAct, SIGNAL(triggered()), this, SLOT(saveMark()));

    // menu.insertItem("Use nm scale", this, SLOT(toggleNmScale()), 0, id);
    // menu.setItemChecked(id, m_useNmScale);
    QAction *toggleNmScaleAct = new QAction(tr("Toggle nm scale"), this);
    toggleNmScaleAct->setStatusTip(tr("Toggle use of nm scale."));
    connect(toggleNmScaleAct, SIGNAL(triggered()), this, SLOT(toggleNmScale()));
    
    id++;
  
    // menu.insertItem("Create 45-degree integer polygon", this, SLOT(create45DegreeIntPoly()));
    QAction *create45DegreeIntPolyAct = new QAction(tr("Create Poly 45 degs."),this);
    create45DegreeIntPolyAct->setStatusTip(tr("Create 45-degree integer polygon."));
    connect(create45DegreeIntPolyAct, SIGNAL(triggered()), this, SLOT(create45DegreeIntPoly()));

    // menu.insertItem("Create arbitrary polygon", this, SLOT(createArbitraryPoly()));
    QAction *createArbitraryPolyAct = new QAction(tr("Create arbitrary polygon"),this);
    connect(createArbitraryPolyAct, SIGNAL(triggered()), this, SLOT(createArbitraryPoly()));

    // menu.insertItem("Delete polygon (Alt-Shift-Mouse)", this, SLOT(deletePoly()));
    QAction *deletePolyAct = new QAction(tr("Delete Poly (Atl+Sh-Mouse)"),this);
    connect(deletePolyAct, SIGNAL(triggered()), this, SLOT(deletePoly()));

    //menu.addSeparator();

    // menu.insertItem("Align mode", this, SLOT(toggleAlignMode()), 0, id);
    // menu.setItemChecked(id, m_alignMode);
    QAction *toggleAlignModeAct = new QAction(tr("Align Mode"),this);
    connect(toggleAlignModeAct, SIGNAL(triggered()), this, SLOT(toggleAlignMode()));
    id++;
    
    // menu.insertItem("Rotate  90 degrees",  this, SLOT(align_rotate90()));
    QAction *align_rotate90Act = new QAction(tr("Rotate  90 degrees"),this);
    if (m_alignMode) {
        connect(align_rotate90Act, SIGNAL(triggered()), this, SLOT(align_rotate90()));

        // menu.insertItem("Rotate 180 degrees",  this, SLOT(align_rotate180()));
        // menu.insertItem("Rotate 270 degrees",  this, SLOT(align_rotate270()));
        // menu.insertItem("Flip against x axis", this, SLOT(align_flip_against_x_axis()));
        // menu.insertItem("Flip against y axis", this, SLOT(align_flip_against_y_axis()));
        // menu.insertItem("Guess alignment", this, SLOT(performAlignmentOfClosePolys()));
        //  menu.addSeparator();
    }
  
    // menu.insertItem("Move polygons (Shift-Mouse)", this, SLOT(turnOnMovePolys()), 0, id);
    // menu.setItemChecked(id, m_movePolys);
    // id++;

    // menu.insertItem("Move vertices (Shift-Mouse)", this, SLOT(turnOnMoveVertices()), 0, id);
    // menu.setItemChecked(id, m_moveVertices);
    // id++;
  
    // menu.insertItem("Move edges (Shift-Mouse)", this, SLOT(turnOnMoveEdges()), 0, id);
    // menu.setItemChecked(id, m_moveEdges);
    // id++;
  
    // menu.insertItem("Insert vertex on edge", this, SLOT(insertVertex()));
    // menu.insertItem("Delete vertex",         this, SLOT(deleteVertex()));
    // menu.insertItem("Copy polygon",          this, SLOT(copyPoly()));
    // menu.insertItem("Paste polygon",         this, SLOT(pastePoly()));
    // menu.insertItem("Reverse orientation",   this, SLOT(reversePoly()));
      
    menu.addAction(saveMarkAct);
    menu.addAction(toggleNmScaleAct);
    menu.addAction(create45DegreeIntPolyAct);
    menu.addAction(createArbitraryPolyAct);
    menu.addAction(deletePolyAct);
    menu.addSeparator();
    menu.addAction(toggleAlignModeAct);

    if (m_alignMode) {
        menu.addAction(align_rotate90Act);
    }
#endif // 0000000000000000000000000000000000000000000000
    menu.exec(E->globalPos());

#endif
  return;
}

void polyView::copyPoly(){

  double min_x, min_y, min_dist;
  int polyVecIndex, polyIndexInCurrPoly, vertIndexInCurrPoly;
  findClosestPolyEdge(// inputs
                      m_menuX, m_menuY, m_polyVec,  
                      // outputs
                      polyVecIndex,
                      polyIndexInCurrPoly,
                      vertIndexInCurrPoly,
                      min_x, min_y, min_dist
                      );
  if (polyVecIndex < 0 || polyIndexInCurrPoly < 0) return;
  
  m_selectedPolyIndices.clear();
  m_selectedPolyIndices[polyVecIndex][polyIndexInCurrPoly] = 1;

  extractMarkedPolys(m_polyVec, m_selectedPolyIndices,  // Inputs
                     m_copiedPolyVec                    // Outputs
                     );

  refreshPixmap();
  
  return;
}

void polyView::translateSelectedPolys(){

  if (getNumElements(m_selectedPolyIndices) == 0){
    popUp("No polygons are selected.");
    return;
  }

  vector<double> inputVec, shiftVec;
  inputVec.clear();
  if ( ! getRealValuesFromGui(// Inputs
                              "Translate selected polygons",
                              "Enter shift_x shift_y", inputVec,
                              // Outputs
                              shiftVec
                              )
       ) return;

  translateSelectedPolys(shiftVec);

  return;
}

void polyView::translateSelectedPolys(std::vector<double> & shiftVec){
  
  if (getNumElements(m_selectedPolyIndices) == 0){
    popUp("No polygons are selected.");
    return;
  }

  if (shiftVec.size() < 2){
    popUp("Invalid translate vector.");
    return;
  }
  shiftVec.resize(2);
  
  shiftMarkedPolys(// Inputs
                   m_selectedPolyIndices,
                   shiftVec[0], shiftVec[1],  
                   // Inputs-outputs
                   m_polyVec
                   );


  printCmd("translate_selected", shiftVec);

  m_highlights.clear();
  saveDataForUndo(false);
  refreshPixmap();
  
  return;
}

void polyView::rotateSelectedPolys(){

  if (getNumElements(m_selectedPolyIndices) == 0){
    popUp("No polygons are selected.");
    return;
  }

  vector<double> inputVec, angle;
  inputVec.clear();
  if ( ! getRealValuesFromGui(// Inputs
                              "Rotate selected polygons",
                              "Enter rotation angle in degrees",
                              inputVec,
                              // Outputs
                              angle
                              )
       ) return;

  rotateSelectedPolys(angle);

  return;
}

void polyView::rotateSelectedPolys(std::vector<double> & angle){
  
  if (getNumElements(m_selectedPolyIndices) == 0){
    popUp("No polygons are selected.");
    return;
  }

  if (angle.size() < 1){
    popUp("Invalid rotation angle.");
    return;
  }
  angle.resize(1);
  
  rotateMarkedPolysAroundCtr(// Inputs
                             m_selectedPolyIndices,  
                             angle[0],  
                             // Inputs-outputs
                             m_polyVec
                             );

  printCmd("rotate_selected", angle);

  m_highlights.clear();
  saveDataForUndo(false);
  refreshPixmap();
  
  return;
}

void polyView::scaleSelectedPolys(){

  if (getNumElements(m_selectedPolyIndices) == 0){
    popUp("No polygons are selected.");
    return;
  }

  vector<double> inputVec, scale;
  inputVec.clear();
  if ( ! getRealValuesFromGui(// Inputs
                              "Scale selected polygons", "Enter scale",
                              inputVec,
                              // Outputs
                              scale
                              )
       ) return;

  scaleSelectedPolys(scale);

  return;
}

void polyView::scaleSelectedPolys(std::vector<double> & scale){
  
  if (getNumElements(m_selectedPolyIndices) == 0){
    popUp("No polygons are selected.");
    return;
  }

  if (scale.size() < 1){
    popUp("Invalid scale value.");
    return;
  }
  scale.resize(1);
  
  scaleMarkedPolysAroundCtr(// Inputs
                             m_selectedPolyIndices,  
                             scale[0],  
                             // Inputs-outputs
                             m_polyVec
                             );

  printCmd("scale_selected", scale);

  m_highlights.clear();
  saveDataForUndo(false);
  refreshPixmap();
  
  return;
}

void polyView::transformSelectedPolys(){

  if (getNumElements(m_selectedPolyIndices) == 0){
    popUp("No polygons are selected.");
    return;
  }

  vector<double> inputVec, T;
  inputVec.clear();
  if ( ! getRealValuesFromGui(// Inputs
                              "Transform selected polygons",
                              "Enter transform matrix a11 a12 a21 a22",
                              inputVec,
                              // Outputs
                              T
                              )
       ) return;

  transformSelectedPolys(T);

  return;
}

void polyView::transformSelectedPolys(std::vector<double> & T){
  
  if (getNumElements(m_selectedPolyIndices) == 0){
    popUp("No polygons are selected.");
    return;
  }

  if (T.size() < 4){
    popUp("Invalid transform matrix.");
    return;
  }
  T.resize(4);
  
  matrix2 M;
  M.a11 = T[0]; M.a12 = T[1]; M.a21 = T[2]; M.a22 = T[3];
  transformMarkedPolysAroundCtr(// Inputs
                                m_selectedPolyIndices, M,
                                // Inputs-outputs
                                m_polyVec
                                );

  printCmd("transform_selected", T);

  m_highlights.clear();
  saveDataForUndo(false);
  refreshPixmap();
  
  return;
}

void polyView::pasteSelectedPolys(){

  extractMarkedPolys(m_polyVec, m_selectedPolyIndices,  // Inputs
                     m_copiedPolyVec                    // Outputs
                     );

  double xll, yll, xur, yur;
  bdBox(m_copiedPolyVec,   // Inputs 
        xll, yll, xur, yur // Outputs
        );
  if (xur < xll || yur < yll) return; // if there are no vertices

  double shift_x = 0.1*(xur - xll);
  double shift_y = 0.1*(yur - yll);

  for (int s = 0; s < (int)m_polyVec.size(); s++){
    m_polyVec[s].appendAndShiftMarkedPolys(// Inputs
                                           m_selectedPolyIndices[s],
                                           shift_x, shift_y
                                           );
  }
  m_highlights.clear();
  saveDataForUndo(false);
  refreshPixmap();
  
  return;
}

void polyView::pastePoly(){
  pasteSelectedPolys();
  return;
}

void polyView::reversePoly(){

  double min_x, min_y, min_dist;
  int polyVecIndex, polyIndexInCurrPoly, vertIndexInCurrPoly;
  findClosestPolyEdge(// inputs
                      m_menuX, m_menuY, m_polyVec,  
                      // outputs
                      polyVecIndex,
                      polyIndexInCurrPoly,
                      vertIndexInCurrPoly,
                      min_x, min_y, min_dist
                      );
  if (polyVecIndex < 0 || polyIndexInCurrPoly < 0) return;
  
  m_polyVec[polyVecIndex].reverseOnePoly(polyIndexInCurrPoly);

  saveDataForUndo(false);
  refreshPixmap();
  
  return;
}

void polyView::refreshPixmap(){

  // Draw the data onto the pixmap instead of the screen
  // directly. Later we'll display the pixmap without redrawing
  // whenever possible for reasons of speed.
  
  m_pixmap = QPixmap(size());
#ifdef USE_QT4_DEFS // QPixmap.fill
  m_pixmap.fill(this, 0, 0);    // Qt5 issues 'this function is deprecated, ignored'
#else
  m_pixmap.fill(Qt::black); // TODO: check this change...
#endif
  
  QPainter paint(&m_pixmap);
  paint.initFrom(this);

  QFont F;
  F.setPointSize(m_prefs.fontSize);
  //F.setStyleStrategy(QFont::NoAntialias);
  paint.setFont(F);

  displayData( &paint );
  update();

  return;
}

void polyView::paintEvent(QPaintEvent *){

  if (m_firstPaintEvent){
    // This will be called the very first time the display is
    // initialized. There must be a better way.
    m_firstPaintEvent = false;
    refreshPixmap(); // Create m_pixmap which we will use as cache
  }

  // Note that we draw from the cached pixmap, instead of redrawing
  // the image from scratch.
  QStylePainter paint(this);
  paint.drawPixmap(0, 0, m_pixmap);

  QColor fgColor = QColor(m_prefs.fgColor.c_str());
  paint.setPen(fgColor);
  paint.drawRect(m_rubberBand.normalized().adjusted(0, 0, -1, -1));

  // Plot the mouse clicks (snapped or not snapped to closest vertex).
  // Do it here since those are temporary, they are supposed
  // to go away when the display changes such as on zoom.
  paint.setPen( QPen(fgColor, m_prefs.lineWidth) );
  paint.setBrush( Qt::NoBrush );
  for (int p = 0; p < (int)m_snappedPoints.size(); p++){
    const QPoint & P = m_snappedPoints[p];
    paint.drawEllipse(P.x() - m_smallLen, P.y() - m_smallLen,
                      2*m_smallLen, 2*m_smallLen
                      );
  }
  for (int p = 0; p < (int)m_nonSnappedPoints.size(); p++){
    const QPoint & P = m_nonSnappedPoints[p];
    paint.drawRect(P.x() - m_smallLen, P.y() - m_smallLen,
                   2*m_smallLen, 2*m_smallLen
                   );
  }

  return;
}

void polyView::drawPolyBeingPlotted(const std::vector<double> & polyX,
                                    const std::vector<double> & polyY,
                                    QPainter * paint){

  if (polyX.size() <= 1) return;

  assert(polyX.size() == polyY.size());
  
  // Draw the current polygon being plotted
  paint->setPen( QPen(QColor((m_prefs.fgColor).c_str()), m_prefs.lineWidth) );
  paint->setBrush( Qt::NoBrush );
  drawPolyLine(polyX, polyY, paint);
  
  // Draw a small rectangle at the start of the polygon to make it easier
  // to tell where to close the polygon.
  int px, py;
  worldToPixelCoords(polyX[0], polyY[0], // inputs
                     px, py // outputs
                     );
  paint->setPen( QPen(QColor((m_prefs.fgColor).c_str()), m_prefs.lineWidth) );
  paint->setBrush( Qt::NoBrush );
  paint->drawRect(px - m_pixelTol, py - m_pixelTol,
                  2*m_pixelTol, 2*m_pixelTol
                  );

  return;
}

void polyView::resizeEvent(QResizeEvent*){
  refreshPixmap();
  return;
}

void polyView::popUp(std::string msg){
  QMessageBox msgBox;
  msgBox.setText(msg.c_str());
  msgBox.exec();
  return;
}

/* 
   getText(QWidget *parent, const QString &title, const QString &label, 
   QLineEdit::EchoMode mode = QLineEdit::Normal, const QString &text = QString(), 
   bool *ok = Q_NULLPTR, Qt::WindowFlags flags = Qt::WindowFlags(), 
   Qt::InputMethodHints inputMethodHints = Qt::ImhNone)

*/

bool polyView::getStringFromGui(std::string title, std::string description,
                                std::string inputStr,
                                std::string & outputStr // output
                                ){

  outputStr = "";

  bool ok = false;
#ifdef USE_QT4_DEFS
  QString text = QInputDialog::getText(title.c_str(), description.c_str(),
                                       QLineEdit::Normal, inputStr.c_str(),
                                       &ok, this );
#else
  QString text = QInputDialog::getText(this, title.c_str(), description.c_str(),
                                       QLineEdit::Normal, inputStr.c_str(),
                                       &ok );
#endif

  if (ok) outputStr = text.toStdString();

  return ok;
}

bool polyView::getRealValuesFromGui(// Inputs
                                    std::string title,
                                    std::string description,
                                    const std::vector<double> & inputVec,
                                    // Outputs
                                    std::vector<double> & values
                                    ){

  values.clear();
  
  string outputStr;

  ostringstream oss;
  oss.precision(16);
  int len = (int)inputVec.size();
  for (int s = 0; s < len - 1; s++) oss << inputVec[s] << " ";
  if (len > 0) oss << inputVec[len - 1];
  string inputStr = oss.str();

  bool ok = getStringFromGui(title, description, inputStr,
                             outputStr
                             );
  
  outputStr = replaceAll(outputStr, ",", " ");
  istringstream ts(outputStr);
  double val;
  while (ts >> val) values.push_back(val);

  return ok;
}

bool polyView::getStringVectorFromGui(std::string title,
                                      std::string description,
                                      std::vector<std::string> & values){

  values.clear();
  string inputStr, outputStr;
  bool ok = getStringFromGui(title, description, inputStr,
                             outputStr // output
                             );
  
  istringstream ts(outputStr);
  string val;
  while (ts >> val) values.push_back(val);

  return ok;
}

void polyView::setLineWidth(){

  vector<double> inputVec, lineWidth;
  if ( !getRealValuesFromGui("Line width", "Enter line width", inputVec,
                             lineWidth) ) return;
  
  if ( !lineWidth.empty() && lineWidth[0] >= 1.0 ){

    int lw = (int) round(lineWidth[0]);

    for (int polyIter = 0; polyIter < (int)m_polyOptionsVec.size(); polyIter++){
      m_polyOptionsVec[polyIter].lineWidth = lw;
    }
    m_prefs.lineWidth = lw;
    
    refreshPixmap();
  
  }else{
    popUp("The line width must be a positive integer.");
  }
  return;
}

void polyView::setGridWidth(){

  vector<double> inputVec, gridWidth;
  if ( !getRealValuesFromGui("Grid linewidth", "Enter grid linewidth",
                             inputVec, gridWidth) ) return;
  
  if (!gridWidth.empty() && gridWidth[0] >= 1.0 ){
    
    int gw = (int) round(gridWidth[0]);
    m_prefs.gridWidth = gw;
    
    refreshPixmap();
  
  }else{
    popUp("The grid linewidth must be a positive integer.");
  }
  return;
}

void polyView::setGridSize(){

  vector<double> inputVec, gridSize;

  // Pass to the GUI the current grid size if set
  inputVec.clear();
  if (m_prefs.gridSize > 0) inputVec.push_back(m_prefs.gridSize);

  if ( !getRealValuesFromGui("Grid size", "Enter grid size", inputVec,
                             gridSize)) return;
         
  if ( !gridSize.empty() && gridSize[0] > 0 ){
    
    double gs = gridSize[0];
    m_prefs.gridSize = gs;
    
    refreshPixmap();
    
  }else{
    popUp("The grid size must be positive.");
  }
  return;
}

void polyView::setGridColor(){

  vector<string> values;
  if (!getStringVectorFromGui("Grid", "Enter grid color",
                              values)) return;

  string gridColor = "";
  if (values.size() > 0) gridColor = values[0];
  if ( QColor(gridColor.c_str()) != QColor::Invalid){
    m_prefs.gridColor = gridColor;
    refreshPixmap();
  }else{
    popUp("Invalid grid color.");
  }
  return;
}

void polyView::setBgColor(){

  vector<string> values;
  if (!getStringVectorFromGui("Background", "Enter background color",
                              values)) return;

  string bgColor = "";
  if (values.size() > 0) bgColor = values[0];
  if ( QColor(bgColor.c_str()) != QColor::Invalid){
    m_prefs.bgColor = bgColor;
    setBgFgColorsFromPrefs();
    refreshPixmap();
  }else{
    popUp("Invalid background color.");
  }
  return;
}

void polyView::setBgFgColorsFromPrefs(){

  // Set the background. Watch for invalid colors.
  string bgColor   = m_prefs.bgColor;
  QColor qtBgColor = QColor(bgColor.c_str());
  if (qtBgColor == QColor::Invalid){
    bgColor   = "black";
    qtBgColor = QColor(bgColor.c_str()); // fallback color
  }
#ifdef USE_QT4_DEFS
  setBackgroundColor(qtBgColor);
#else
  // TODO: ????????????????
#endif
  string fgColor = m_prefs.fgColor;
  if ( QColor(fgColor.c_str()) == QColor::Invalid ){
    fgColor = "white";
  }

  // Make sure bg and fg have different colors
  if (bgColor == fgColor){
    if (bgColor == "black") fgColor = "white";
    else                    fgColor = "black";
  }

  // Update the preferences after doing the logic above
  m_prefs.bgColor = bgColor;
  m_prefs.fgColor = fgColor;

  // While unnecessary, also update the preferences for each
  // polygon file, for consistency with other preferences per file.
  for (int s = 0; s < (int)m_polyOptionsVec.size(); s++){
    m_polyOptionsVec[s].bgColor = bgColor; 
    m_polyOptionsVec[s].fgColor = fgColor; 
  }

  return;
}


void polyView::translatePolys(){

  vector<double> inputVec, shiftVec;
  if ( getRealValuesFromGui("Translate polygons", "Enter shift_x and shift_y",
                            inputVec,
                            shiftVec) ){
    translatePolys(shiftVec);
  }
  return;
}

void polyView::rotatePolys(){

  vector<double> inputVec, angle;
  if ( getRealValuesFromGui("Rotate polygons",
                            "Enter rotation angle in degrees",
                            inputVec,
                            angle) ){
    rotatePolys(angle);
  }
  return;
}

void polyView::scalePolys(){

  vector<double> inputVec, scale;
  if ( getRealValuesFromGui("Scale polygons", "Enter scale factor",
                            inputVec,
                            scale) ){
    scalePolys(scale);
  }
  return;
}

void polyView::translatePolys(std::vector<double> & shiftVec){

  if (shiftVec.size() < 2){
    popUp("Invalid shift_x and shift_y values.");
    return;
  }
  shiftVec.resize(2);
  
  printCmd("translate", shiftVec);
  for (int vi  = 0; vi < (int)m_polyVec.size(); vi++){
    m_polyVec[vi].shift(shiftVec[0], shiftVec[1]);
  }

  saveDataForUndo(true);
  resetView();
  
  return;
}

void polyView::rotatePolys(std::vector<double> & angle){

  if (angle.size() < 1){
    popUp("Invalid rotation angle.");
    return;
  }
  angle.resize(1);
  
  printCmd("rotate", angle);
  for (int vi  = 0; vi < (int)m_polyVec.size(); vi++){
    m_polyVec[vi].rotate(angle[0]);
  }

  saveDataForUndo(true);
  resetView();
  
  return;
}

void polyView::scalePolys(std::vector<double> & scale){

  if (scale.size() < 1){
    popUp("Invalid scale factor.");
    return;
  }
  scale.resize(1);
  
  printCmd("scale", scale);
  for (int vi  = 0; vi < (int)m_polyVec.size(); vi++){
    m_polyVec[vi].scale(scale[0]);
  }

  saveDataForUndo(true);
  resetView();
  
  return;
}

void polyView::transformPolys(std::vector<double> & M){

  if (M.size() < 6){
    popUp("Invalid linear transform.");
    return;
  }

  bool resetViewOnUndo = !m_alignMode;
  
  int end = m_polyVec.size();
  if (m_alignMode) end = min(end, 1);
    
  for (int vi = 0; vi < end; vi++){
    m_polyVec[vi].applyTransform(M[0], M[1], M[2], M[3], M[4], M[5], m_T);
  }

  if (m_alignMode) m_totalT = composeTransforms(m_T, m_totalT);
  
  m_T.print();

  saveDataForUndo(resetViewOnUndo);

  if (resetViewOnUndo) resetView();
  else                 refreshPixmap();
  
  return;
}

void polyView::addPolyVert(int px, int py){

  // Add a point to the polygon being drawn or stop drawing and append
  // the drawn polygon to the list of polygons.

  double wx, wy;
  pixelToWorldCoords(px, py, wx, wy);

  double wtol = pixelToWorldDist(m_pixelTol);
  int pSize   = m_currPolyX.size();
  
  if (pSize <= 0 ||
      distance(m_currPolyX[0], m_currPolyY[0], wx, wy) > wtol
      ){

    // We did not arrive at the starting point of the polygon being
    // drawn. Add the current point.
    
    m_currPolyX.push_back(wx);
    m_currPolyY.push_back(wy);
    pSize = m_currPolyX.size();
    if (m_snapPolyTo45DegreeIntGrid){
      bool isClosedPolyLine = false;
      snapPolyLineTo45DegAngles(isClosedPolyLine, pSize,
                                vecPtr(m_currPolyX), vecPtr(m_currPolyY));
    }

    // This will call paintEvent which will draw the current poly line
    update();

    return;
  }

  // We arrived at the starting point of the polygon being drawn. Stop
  // adding points and append the current polygon.
  
  if (m_snapPolyTo45DegreeIntGrid){
    bool isClosedPolyLine = true;
    snapPolyLineTo45DegAngles(isClosedPolyLine, pSize,
                              vecPtr(m_currPolyX), vecPtr(m_currPolyY));
  }
  
  // Get the layer and color from the closest existing polygon
  double minX = DBL_MAX, minY = DBL_MAX, minDist = DBL_MAX;
  int minVecIndex, minPolyIndex, minVertIndex;
  findClosestPolyEdge(// inputs
                      m_currPolyX[0], m_currPolyY[0],
                      m_polyVec,  
                      // outputs
                      minVecIndex, minPolyIndex, minVertIndex, 
                      minX, minY, minDist
                      );
  string color, layer;
  if (minVecIndex >= 0 && minPolyIndex >= 0){
    const vector<string> & layers = m_polyVec[minVecIndex].get_layers();
    const vector<string> & colors = m_polyVec[minVecIndex].get_colors();
    color = colors.at(minPolyIndex);
    layer = layers.at(minPolyIndex);
  }else{
    // No other polygons to borrow layer and color info from. Just use
    // some defaults then.
    color = m_prefs.cmdLineColor;
    layer = "";
  }

  // Form the new polygon
  dPoly P;
  bool isPolyClosed = (m_prefs.isPolyClosed != forceNonClosedPoly);
  P.reset();
  P.appendPolygon(pSize, vecPtr(m_currPolyX), vecPtr(m_currPolyY),
                  isPolyClosed, color, layer);

  appendToPolyVec(P);

  // Reset
  m_createPoly = false;
  m_currPolyX.clear();
  m_currPolyY.clear();
  setStandardCursor();
  refreshPixmap();
    
  return;
}

void polyView::appendToPolyVec(const dPoly & P){

  // Append the new polygon to the list of polygons. If we have several
  // clips already, append it to the last clip. If we have no clips,
  // create a new clip.
  if (m_polyVec.size() == 0){
    m_polyVec.push_back(P);
    m_polyOptionsVec.push_back(m_prefs);
    string fileName = "poly" + num2str(m_polyVec.size() - 1) + ".xg";
    m_polyOptionsVec.back().polyFileName = fileName;
  }else{
    m_polyVec.back().appendPolygons(P);
  }

  saveDataForUndo(false);

  return;
}

void polyView::drawPolyLine(const std::vector<double> & polyX,
                            const std::vector<double> & polyY,
                            QPainter * paint){

  dPoly  polyLine;
  bool   isPolyClosed = false;
  string color        = m_prefs.fgColor.c_str();
  string layer        = "";
  polyLine.setPolygon(polyX.size(), vecPtr(polyX), vecPtr(polyY),
                      isPolyClosed, color, layer
                      );

  bool plotPoints = false, plotEdges = true, plotFilled = false;
  int drawVertIndex = 0;
  vector< vector<int> > textOnScreenGrid; textOnScreenGrid.clear();
  bool showAnno = false;
  plotDPoly(plotPoints, plotEdges, plotFilled, showAnno, m_prefs.lineWidth, 
            drawVertIndex, textOnScreenGrid, paint, polyLine
            );

  return;
}

void polyView::createHighlightWithPixelInputs(int pxll, int pyll, int pxur, int pyur){
  
  double xll, yll, xur, yur;
  pixelToWorldCoords(pxll, pyll, // inputs
                     xll, yll    // outputs
                     );
  pixelToWorldCoords(pxur, pyur, // inputs
                     xur, yur    // outputs
                     );

  createHighlightWithRealInputs(xll, yll, xur, yur);

  return;
}

void polyView::createHighlightWithRealInputs(double xll, double yll, double xur, double yur){
  
  dPoly R;
  bool isPolyClosed = true;
  string color = m_prefs.fgColor, layer = "";
  R.setRectangle(min(xll, xur), min(yll, yur), max(xll, xur), max(yll, yur),
                 isPolyClosed, color, layer);
  m_highlights.push_back(R);
  markPolysInHlts(m_polyVec, m_highlights, // Inputs
                  m_selectedPolyIndices    // Outputs
                  );
  turnOnMovePolys();
  saveDataForUndo(false);
  
  return;
}

#ifdef USE_QT4_DEFS
void polyView::printCurrCoords(const Qt::ButtonState & state, // input
                               int & currX, int  & currY      // in-out
                               )
{
    // Snap or not the current point to the closest polygon vertex
    // and print its coordinates.
    bool set_width = false;   // try without width
    double s;
    string unit;
    if (m_useNmScale) {
        s    = m_nmScale;
        unit = " (nm):";
    } else {
       s    = 1.0;
       unit = "";
    }

    int prec = 6, wid = prec + 6;
    if (set_width) {  // prefer this off, and use max 15
        cout.precision(prec);
        cout.setf(ios::floatfield);
    }
    double wx, wy;
    pixelToWorldCoords(currX, currY, wx, wy);
    int len = 2*m_smallLen + 2*m_prefs.lineWidth; // big enough

    if (state == (int)Qt::LeftButton) {
        // Snap to the closest vertex with the left mouse button.
        double min_x, min_y, min_dist;
        int polyVecIndex, polyIndexInCurrPoly, vertIndexInCurrPoly;
        findClosestPolyVertex(// inputs
                          wx, wy, m_polyVec,
                          // outputs
                          polyVecIndex,
                          polyIndexInCurrPoly,
                          vertIndexInCurrPoly,
                          min_x, min_y, min_dist
                          );
        wx = min_x; 
        wy = min_y;
        worldToPixelCoords(wx, wy,      // inputs
                       currX, currY // outputs
                       );
    
        // Save the point to plot. Call update to paint the point.
        m_snappedPoints.push_back(QPoint(currX, currY));
        update(currX - len, currY - len, 2*len, 2*len);
    
    } else if (state == ( (int)Qt::LeftButton | (int)Qt::AltModifier )
            ||
            state == ((int)Qt::MidButton)
            ) 
    {
        // Don't snap with the alt-left button or the middle button.
    
        // Save the point to plot. Call update to paint the point.
        m_nonSnappedPoints.push_back(QPoint(currX, currY));
        int len = 2*m_smallLen + 2*m_prefs.lineWidth; // big enough
        update(currX - len, currY - len, 2*len, 2*len);
      
    }
    // hmmm, do not like this fixed width display TODO: make this a config value
    if (set_width) {
        cout << "Point" << unit << " ("
           << setw(wid) << s*wx << ", "
           << setw(wid) << s*wy << ")";
        if (m_prevClickExists){
            cout  << " dist from prev: ("
              << setw(wid) << s*(wx - m_prevClickedX) << ", "
              << setw(wid) << s*(wy - m_prevClickedY)
              << ") Euclidean: "
              << setw(wid) << s*sqrt( (wx - m_prevClickedX)*(wx - m_prevClickedX)
                                      + 
                                      (wy - m_prevClickedY)*(wy - m_prevClickedY)
                                      );
        }
    } else {
        cout.precision(15); // set maximum precision
        cout << "Point: "
           << wx << ", "
           << wy;
        if (m_prevClickExists) {
            cout  << " from prev: " << unit << " "
              << s*sqrt( (wx - m_prevClickedX)*(wx - m_prevClickedX) + 
                         (wy - m_prevClickedY)*(wy - m_prevClickedY) );
        }
    }
    cout << endl;
  
    m_prevClickExists = true;
    m_prevClickedX    = wx;
    m_prevClickedY    = wy;

    return;
}
#else
    // TODO: port coordinates output
void polyView::printCurrCoords(const Qt::KeyboardModifiers & keymods, // input
                               int & currX, int  & currY      // in-out
                               )
{
    size_t state = 0;
    if (m_mouseButs & Qt::LeftButton) {
        state = Qt::LeftButton;
        if (m_mouseMods & Qt::AltModifier) {
            state |= Qt::AltModifier;
        }
    } else if (m_mouseButs & Qt::MidButton) {
        state = Qt::MidButton;
    } else if (m_mouseButs & Qt::RightButton) {
        state = Qt::RightButton;
        return; // this is the context menu - no cordinate output
    }

    // Snap or not the current point to the closest polygon vertex
    // and print its coordinates.
    bool set_width = false;   // try without width
    double s;
    string unit;
    if (m_useNmScale) {
        s   = m_nmScale;    // TODO: Not USED - file input 'scale.txt' disabled
        unit = " (nm):";    // Assumes wsg84 lon lat coord, and uses Haversine to compute distance
    } else {
       s    = 1.0;
       unit = "RMS:";
    }

    int prec = 6, wid = prec + 6;
    if (set_width) {  // prefer this off, and use max 15
        cout.precision(prec);
        cout.setf(ios::floatfield);
    }
    double wx, wy;
    pixelToWorldCoords(currX, currY, wx, wy);
    int len = 2*m_smallLen + 2*m_prefs.lineWidth; // big enough

    if (state == (int)Qt::LeftButton) {
        // Snap to the closest vertex with the left mouse button.
        double min_x, min_y, min_dist;
        int polyVecIndex, polyIndexInCurrPoly, vertIndexInCurrPoly;
        findClosestPolyVertex(// inputs
                          wx, wy, m_polyVec,
                          // outputs
                          polyVecIndex,
                          polyIndexInCurrPoly,
                          vertIndexInCurrPoly,
                          min_x, min_y, min_dist
                          );
        wx = min_x; 
        wy = min_y;
        worldToPixelCoords(wx, wy,      // inputs
                       currX, currY // outputs
                       );
    
        // Save the point to plot. Call update to paint the point.
        m_snappedPoints.push_back(QPoint(currX, currY));
        update(currX - len, currY - len, 2*len, 2*len);
    
    } else if (state == ( (int)Qt::LeftButton | (int)Qt::AltModifier )
            ||
            state == ((int)Qt::MidButton)
            ) 
    {
        // Don't snap with the alt-left button or the middle button.
    
        // Save the point to plot. Call update to paint the point.
        m_nonSnappedPoints.push_back(QPoint(currX, currY));
        int len = 2*m_smallLen + 2*m_prefs.lineWidth; // big enough
        update(currX - len, currY - len, 2*len, 2*len);
      
    }
    //////////////////////////////////////////////////////////////////////////////
    // hmmm, do not like this fixed width display TODO: make this a config value
    if (set_width) {
        cout << "Point" << unit << " ("
           << setw(wid) << s*wx << ", "
           << setw(wid) << s*wy << ")";
        if (m_prevClickExists){
            cout  << " dist from prev: ("
              << setw(wid) << s*(wx - m_prevClickedX) << ", "
              << setw(wid) << s*(wy - m_prevClickedY)
              << ") Euclidean: "
              << setw(wid) << s*sqrt( (wx - m_prevClickedX)*(wx - m_prevClickedX)
                                      + 
                                      (wy - m_prevClickedY)*(wy - m_prevClickedY)
                                      );
        }
    } else {

        cout.precision(15); // set maximum precision
        cout << "Point: "
           << wx << ", "
           << wy;
        if (m_prevClickExists) {
            double dist;
            if (!m_useNmScale) {
                // pythag sqrt(sum of shares) - no prob
                // ok for quite small distances - difficulty is scaling
                // Previous was through an external scale.txt file with 'scale 1234'!
                dist = s*sqrt( (wx - m_prevClickedX)*(wx - m_prevClickedX) + 
                               (wy - m_prevClickedY)*(wy - m_prevClickedY) );
            } else {
                // Haversine
                dist = DistanceInMeters(m_prevClickedY, m_prevClickedX, wy, wx);
                if (dist < 1000)
                    unit = "(m)";
                else {
                    dist /= 1000;
                    dist *= KM2NM;
                    unit = "(nm)";
                }
            }
            cout  << " from prev: " << unit << " " << dist;
        }
    }
    cout << endl;
  
    m_prevClickExists = true;
    m_prevClickedX    = wx;
    m_prevClickedY    = wy;

    return;
}
#endif

void polyView::updateRubberBand(QRect & R){
  
  QRect rect = R.normalized();
  update(rect.left(), rect.top(),    rect.width(), 1             );
  update(rect.left(), rect.top(),    1,            rect.height() );
  update(rect.left(), rect.bottom(), rect.width(), 1             );
  update(rect.right(), rect.top(),   1,            rect.height() );
  
  return;
}

void polyView::pixelToWorldCoords(int px, int py,
                                  double & wx, double & wy){

  // Compensate for the Qt's origin being in the upper-left corner
  // instead of the lower-left corner.
  py = m_screenWidY - py;

  wx = px*m_pixelSize + m_viewXll;
  wy = py*m_pixelSize + m_viewYll;

}

void polyView::worldToPixelCoords(double wx, double wy,
                                  int & px,  int & py){

  px = iround((wx - m_viewXll)/m_pixelSize);
  py = iround((wy - m_viewYll)/m_pixelSize);
  
  // Compensate for the Qt's origin being in the upper-left corner
  // instead of the lower-left corner.
  py = m_screenWidY - py;
  
}

void polyView::drawOneVertex(int x0, int y0, QColor color, int lineWidth,
                             int drawVertIndex, QPainter * paint){

  // Draw a vertex as a small shape (a circle, rectangle, triangle)
  
  // Use variable size shapes to distinguish better points on top of
  // each other
  int len = 3*(drawVertIndex+1); 
  len = min(len, 8); // limit how big this can get
  
  paint->setPen( QPen(color, lineWidth) );

  int numTypes = 4;
  if (drawVertIndex < 0){
    
    // This will be reached only for the case when a polygon
    // is so small that it collapses into a point.
    len = lineWidth;
    paint->setBrush( color );
    paint->drawRect(x0 - len, y0 - len, 2*len, 2*len);
    
  } else if (drawVertIndex%numTypes == 0){
    
    // Draw a small empty ellipse
    paint->setBrush( Qt::NoBrush );
    paint->drawEllipse(x0 - len, y0 - len, 2*len, 2*len);
    
  }else if (drawVertIndex%numTypes == 1){
    
    // Draw an empty square
    paint->setBrush( Qt::NoBrush );
    paint->drawRect(x0 - len, y0 - len, 2*len, 2*len);
    
  }else if (drawVertIndex%numTypes == 2){
    
    // Draw an empty triangle
    paint->setBrush( Qt::NoBrush );
    paint->drawLine(x0 - len, y0 - len, x0 + len, y0 - len);
    paint->drawLine(x0 - len, y0 - len, x0 + 0,   y0 + len);
    paint->drawLine(x0 + len, y0 - len, x0 + 0,   y0 + len);
    
  }else{
    
    // Draw an empty reversed triangle
    paint->setBrush( Qt::NoBrush );
    paint->drawLine(x0 - len, y0 + len, x0 + len, y0 + len);
    paint->drawLine(x0 - len, y0 + len, x0 + 0,   y0 - len);
    paint->drawLine(x0 + len, y0 + len, x0 + 0,   y0 - len);
    
  }
  
  return;
}

void polyView::drawMark(int x0, int y0, QColor color, int lineWidth,
                        QPainter * paint){
  
  int len = 6;

  paint->setBrush( Qt::NoBrush );
  paint->setPen( QPen(color, lineWidth) );

  // Draw a cross
  paint->drawLine(x0 - len, y0 - len, x0 + len, y0 + len);
  paint->drawLine(x0 - len, y0 + len, x0 + len, y0 - len);
  
}

void polyView::toggleAnno(){
  m_showAnnotations   = !m_showAnnotations;
  m_showVertIndexAnno = false;
  m_showLayerAnno     = false;
#ifdef ADD_PREF_INI
  utils::writeSetting("m_showAnnotations",m_showAnnotations);
  utils::writeSetting("m_showVertIndexAnno",m_showVertIndexAnno);
  utils::writeSetting("m_showLayerAnno",m_showLayerAnno);
#endif // ADD_PREF_INI
  refreshPixmap();
}

void polyView::toggleVertIndexAnno(){
  m_showVertIndexAnno = !m_showVertIndexAnno;
  m_showAnnotations   = false;
  m_showLayerAnno     = false;
#ifdef ADD_PREF_INI
  utils::writeSetting("m_showAnnotations",m_showAnnotations);
  utils::writeSetting("m_showVertIndexAnno",m_showVertIndexAnno);
  utils::writeSetting("m_showLayerAnno",m_showLayerAnno);
#endif // ADD_PREF_INI
  refreshPixmap();
}

void polyView::toggleLayerAnno(){
  m_showLayerAnno     = !m_showLayerAnno;
  m_showAnnotations   = false;
  m_showVertIndexAnno = false;
#ifdef ADD_PREF_INI
  utils::writeSetting("m_showAnnotations",m_showAnnotations);
  utils::writeSetting("m_showVertIndexAnno",m_showVertIndexAnno);
  utils::writeSetting("m_showLayerAnno",m_showLayerAnno);
#endif // ADD_PREF_INI
  refreshPixmap();
}

void polyView::toggleFilled(){
  m_showFilledPolys = !m_showFilledPolys;
  refreshPixmap();
}

void polyView::toggleShowGrid(){
  m_prefs.isGridOn = !m_prefs.isGridOn;
  refreshPixmap();
}

void polyView::toggleDifferentColors(){

  // Color the polys in different colors to distinguish them better
  
  if (m_diffColorsMode){
    // Turn off diff color mode
    m_diffColorsMode    = false;
    m_polyVec           = m_polyVecBk;
    m_polyOptionsVec    = m_polyOptionsVecBk;

    refreshPixmap();
    return;
  }

  // Turn on diff color mode

  // First turn off diff mode
  if (m_polyDiffMode) toggleShowPolyDiff();
  
  assert( m_polyVec.size() == m_polyOptionsVec.size() );
  
  m_diffColorsMode = true;

  string colors[] = {"red", "blue", "yellow", "white", "green", "cyan", "magenta"};
  int numColors = sizeof(colors)/sizeof(string);

  // Back up the current settings before entering poly diff mode
  // to be able to restore them later.
  m_polyVecBk        = m_polyVec;
  m_polyOptionsVecBk = m_polyOptionsVec;

  cout << "Changing the polygons colors" << endl;
  for (int pIter = 0; pIter < (int)m_polyVec.size(); pIter++){
    string color = colors[pIter % numColors];
    m_polyVec[pIter].set_color(color);
    cout << color << "\t" << m_polyOptionsVec[pIter].polyFileName << endl;
  }
  
  refreshPixmap();
}

void polyView::toggleShowPolyDiff(){

  // Show the differences of two polygons as points
  
  printCmd("poly_diff");

  if (m_polyDiffMode){
    // Turn off diff mode
    m_polyDiffMode      = false;
    m_polyVec           = m_polyVecBk;
    m_polyOptionsVec    = m_polyOptionsVecBk;

    // See polyView::plotDiff() for explanation.
    m_distVec.clear();
    m_indexOfDistToPlot = -1; 

    refreshPixmap();
    return;
  }

  // Turn on diff mode
  
  // First turn off diff color mode
  if (m_diffColorsMode) toggleDifferentColors();

  assert( m_polyVec.size() == m_polyOptionsVec.size() );
  
  if (m_polyVec.size() < 2){
    popUp("Must have two polygon files to diff.");
    return;
  }

  if (m_polyVec.size() > 2){
    cout << "Showing the differences of the first two polygon files "
         << "and ignoring the rest" << endl;
  }

  m_polyDiffMode = true;

  // Back up the current settings before entering poly diff mode
  // to be able to restore them later.
  m_polyVecBk        = m_polyVec;
  m_polyOptionsVecBk = m_polyOptionsVec;

  m_polyVec.resize(4);
  m_polyOptionsVec.resize(4);

  string color1 = "red", color2 = "blue", layer1 = "", layer2 = "";
  
  dPoly & P = m_polyVec[0]; // alias
  dPoly & Q = m_polyVec[1]; // alias
  vector<dPoint>  vP, vQ;

  findPolyDiff(P, Q,  // inputs
               vP, vQ // outputs
               );

  cout << "Changing the polygons colors to " << color1 << " and "
       << color2 << " in show poly diff mode"<< endl;
  
  P.set_color(color1);
  Q.set_color(color2);
  
  m_polyVec[2].set_pointCloud(vP, color1, layer1);
  m_polyVec[3].set_pointCloud(vQ, color2, layer2);

  m_polyOptionsVec[2].plotAsPoints = true;
  m_polyOptionsVec[3].plotAsPoints = true;

  m_polyOptionsVec[2].polyFileName = "diff1.xg";
  m_polyOptionsVec[3].polyFileName = "diff2.xg";
  
  refreshPixmap();
}

void polyView::plotNextDiff(){
  plotDiff(1);
}

void polyView::plotPrevDiff(){
  plotDiff(-1);
}

void polyView::plotDiff(int direction){

  // For every vertex in m_polyVec[0], find the distance to the
  // closest point on the closest edge of m_polyVec[1], and the
  // segment achieving this shortest distance. Do the same with the
  // polygons reversed. We sort all these distances in decreasing
  // order and store them in m_distVec. The user will navigate over
  // these segments to see how different the polygon clips are.

  if ( !m_polyDiffMode ) return;

  // The current segment to plot
  m_segX.clear(); m_segY.clear();

  assert(direction == 1 || direction == -1);
  
  if (m_distVec.size() == 0 ){
    assert( m_polyVec.size() >= 2);
    findDistanceBwPolys(// inputs
                        m_polyVec[0], m_polyVec[1], 
                        // outputs
                        m_distVec
                        );
  }

  if (m_indexOfDistToPlot < 0){
    if (direction > 0) m_indexOfDistToPlot = -1;
    else               m_indexOfDistToPlot = 0;
  }
  m_indexOfDistToPlot += direction;

  int len = m_distVec.size();
  if (len > 0){
    m_indexOfDistToPlot = (m_indexOfDistToPlot + len) % len;
  }else{
    m_indexOfDistToPlot = -1; // Nothing to plot
  }
    
  if (m_indexOfDistToPlot < 0 || m_indexOfDistToPlot >= len) return;

  // The segment to plot
  segDist S = m_distVec[m_indexOfDistToPlot];
  m_segX.push_back(S.begx); m_segX.push_back(S.endx);
  m_segY.push_back(S.begy); m_segY.push_back(S.endy);

  cout << "Distance: " << S.dist << endl;

  // Set up the view so that it is centered at the midpoint of this
  // segment.
  double midX = (m_segX[0] + m_segX[1])/2.0;
  double midY = (m_segY[0] + m_segY[1])/2.0;
  m_viewXll  = midX - m_viewWidX/2.0;
  m_viewYll  = midY - m_viewWidY/2.0;
  
  refreshPixmap(); // Will call plotDistBwPolyClips(...)
}


void polyView::plotDistBwPolyClips( QPainter *paint ){

  // This is to be called in poly diff mode only. Plot the current
  // segment/distance between clips of polygons. See
  // polyView::plotDiff() for more info.
  
  if ( !m_polyDiffMode ) return;
  drawPolyLine(m_segX, m_segY, paint);
  return;
}

void polyView::turnOnMovePolys(){

  m_movePolys    = true;
  m_moveVertices = false;
  m_moveEdges    = false;
  m_alignMode    = false;
  if (m_toggleShowPointsEdges == m_showPointsEdges)
    m_toggleShowPointsEdges = m_toggleShowPointsEdgesBk;
  
  refreshPixmap();
  return;
}

void polyView::turnOnMoveVertices(){

  m_movePolys    = false;
  m_moveVertices = true;
  m_moveEdges    = false;
  m_alignMode    = false;
  
  if (m_toggleShowPointsEdges != m_showPointsEdges){
    m_toggleShowPointsEdgesBk = m_toggleShowPointsEdges;
    m_toggleShowPointsEdges   = m_showPointsEdges;
  }
  
  refreshPixmap();
  return;
}

void polyView::turnOnMoveEdges(){

  m_movePolys    = false;
  m_moveVertices = false;
  m_moveEdges    = true;
  m_alignMode    = false;
  
  if (m_toggleShowPointsEdges != m_showPointsEdges){
    m_toggleShowPointsEdgesBk = m_toggleShowPointsEdges;
    m_toggleShowPointsEdges   = m_showPointsEdges;
  }
  
  refreshPixmap();
  return;
}

void polyView::toggleAlignMode(){
  
  m_alignMode = !m_alignMode;

  if (m_alignMode){
    
    if (m_polyVec.size() < 2){
      popUp("Must have two polygon files to align.");
      m_alignMode = false;
      return;
    }

    if (m_toggleShowPointsEdges == m_showPointsEdges)
      m_toggleShowPointsEdges = m_toggleShowPointsEdgesBk;
    
    m_movePolys    = false;
    m_moveVertices = false;
    m_moveEdges    = false;

    m_highlights.clear();
    markPolysInHlts(m_polyVec, m_highlights, // Inputs
                    m_selectedPolyIndices    // Outputs
                    );
    
    m_totalT.reset();
  }else{
    cout << "\nCombined transform:" << endl;
    m_totalT.print();
    cout << endl;
  }
  
  refreshPixmap();
  return;
}

void polyView::align_rotate90(){
  assert(m_alignMode);
  m_polyVec[0].applyTransformAroundBdBoxCenter(0, -1, 1, 0, m_T);
  m_T.print();
  m_totalT = composeTransforms(m_T, m_totalT);
  saveDataForUndo(false);
  refreshPixmap();
}

void polyView::align_rotate180(){
  assert(m_alignMode);
  m_polyVec[0].applyTransformAroundBdBoxCenter(-1, 0, 0, -1, m_T);
  m_T.print();
  m_totalT = composeTransforms(m_T, m_totalT);
  saveDataForUndo(false);
  refreshPixmap();
}

void polyView::align_rotate270(){
  assert(m_alignMode);
  m_polyVec[0].applyTransformAroundBdBoxCenter(0, 1, -1, 0, m_T);
  m_T.print();
  m_totalT = composeTransforms(m_T, m_totalT);
  saveDataForUndo(false);
  refreshPixmap();
}

void polyView::align_flip_against_y_axis(){
  assert(m_alignMode);
  m_polyVec[0].applyTransformAroundBdBoxCenter(-1, 0, 0, 1, m_T);
  m_T.print();
  m_totalT = composeTransforms(m_T, m_totalT);
  saveDataForUndo(false);
  refreshPixmap();
}

void polyView::align_flip_against_x_axis(){
  assert(m_alignMode);
  m_polyVec[0].applyTransformAroundBdBoxCenter(1, 0, 0, -1, m_T);
  m_T.print();
  m_totalT = composeTransforms(m_T, m_totalT);
  saveDataForUndo(false);
  refreshPixmap();
}

void polyView::performAlignmentOfClosePolys(){
  assert(m_alignMode);
  assert(m_polyVec.size() >= 2);
  alignPoly1ToPoly2(m_polyVec[0], m_polyVec[1], m_T);
  m_T.print();
  m_totalT = composeTransforms(m_T, m_totalT);
  saveDataForUndo(false);
  refreshPixmap();
}

void polyView::create45DegreeIntPoly(){

  // This flag will change the behavior of mouseReleaseEvent() so that
  // we can start adding points to the polygon with the mouse.
  m_createPoly                = true;

  m_snapPolyTo45DegreeIntGrid = true;
  
  setPolyDrawCursor();
}

void polyView::createArbitraryPoly(){

  // This flag will change the behavior of mouseReleaseEvent() so that
  // we can start adding points to the polygon with the mouse.
  m_createPoly                = true;
  
  m_snapPolyTo45DegreeIntGrid = false;
  
  setPolyDrawCursor();
}

void polyView::insertVertex(){

  if (m_polyVec.size() == 0) return;

  turnOnMoveVertices();
  
  double min_x, min_y, min_dist;
  findClosestPolyEdge(// inputs
                      m_menuX, m_menuY, m_polyVec,  
                      // outputs
                      m_polyVecIndex,
                      m_polyIndexInCurrPoly,
                      m_vertIndexInCurrPoly,
                      min_x, min_y, min_dist
                      );
  
  if (m_polyVecIndex        < 0 ||
      m_polyIndexInCurrPoly < 0 ||
      m_vertIndexInCurrPoly < 0) return;
  
  // Need +1 below as we insert AFTER current vertex.
  m_polyVec[m_polyVecIndex].insertVertex(m_polyIndexInCurrPoly,
                                         m_vertIndexInCurrPoly + 1,
                                         min_x, min_y
                                         );
  
  saveDataForUndo(false);
  refreshPixmap();
  return;
}

void polyView::deleteVertex(){

  if (m_polyVec.size() == 0) return;
  
  turnOnMoveVertices();

  double min_x, min_y, min_dist;
  findClosestPolyVertex(// inputs
                        m_menuX, m_menuY, m_polyVec,
                        // outputs
                        m_polyVecIndex,
                        m_polyIndexInCurrPoly,
                        m_vertIndexInCurrPoly,
                        min_x, min_y, min_dist
                        );
  
  if (m_polyVecIndex        < 0 ||
      m_polyIndexInCurrPoly < 0 ||
      m_vertIndexInCurrPoly < 0) return;
  
  m_polyVec[m_polyVecIndex].eraseVertex(m_polyIndexInCurrPoly,
                                        m_vertIndexInCurrPoly
                                        );
  
  saveDataForUndo(false);
  refreshPixmap();

  return;
}

void polyView::deletePoly(){

  if (m_polyVec.size() == 0) return;
  
  int minVecIndex, minPolyIndex, minVertIndex;
  double minX = DBL_MAX, minY = DBL_MAX, minDist = DBL_MAX;
  findClosestPolyEdge(// inputs
                      m_menuX, m_menuY,
                      m_polyVec,  
                      // outputs
                      minVecIndex, minPolyIndex, minVertIndex, 
                      minX, minY, minDist
                      );
  
  if (minVecIndex >= 0 && minPolyIndex >= 0){
    m_polyVec[minVecIndex].eraseOnePoly(minPolyIndex);
  }

  saveDataForUndo(false);
  refreshPixmap();
  
  return;
}

void polyView::saveMark(){

  // When saving the mark don't overwrite existing marks
  int markIndex = 0;
  string markFile;
  while(1){
    markIndex++;
    markFile = "mark" + num2str(markIndex) + ".xg";
    ifstream mark(markFile.c_str());
    if (!mark) break;
  }

  plotMark(m_menuX, m_menuY);

  cout << "Saving the mark to " << markFile << endl;
  ofstream mark(markFile.c_str());
  mark << "color = white" << endl;
  mark << m_menuX << ' ' << m_menuY << endl;
  mark << "NEXT" << endl;
  mark.close();
  return;
}

void polyView::plotMark(double x, double y){
  cout << "mark " << x << ' ' << y << endl;
  m_markX.resize(1); m_markX[0] = x;
  m_markY.resize(1); m_markY[0] = y;
  refreshPixmap();
  return;
}

void polyView::toggleNmScale(){

  m_useNmScale = !m_useNmScale;
  if (!m_useNmScale){
      //  Pythagoras - root sum sides squared
    cout << "Using the dbu scale" << endl;
    return;
  }

#if 0 // 00000000000000000000000000000000000000000000000000000000000
  ifstream scaleHandle(m_nmScaleFile.c_str());
  if (!scaleHandle){
    cerr << "File " << m_nmScaleFile << " does not exist" << endl;
    m_useNmScale = false;
    return;
  }

  string dummy;
  if (! (scaleHandle >> dummy >> m_nmScale) ){
    cerr << "Could not read the nm scale factor from "
         << m_nmScaleFile << endl;
    cerr << "Syntax is: scale 1.5" << endl;
    m_useNmScale = false;
    return;
  }

  if (m_nmScale <= 0.0){
    cerr << "The nm scale factor must be greater than 0" << endl;
    m_useNmScale = false;
    return;
  }
#endif // 0000000000000000000000000000000000

  cout << "Using the nm scale factor " << m_nmScale << endl;

  return;
}

void polyView::createHlt(){
  popUp("To create a highlight use Control-Mouse.");
  return;
}

void polyView::moveSelectedPolys(){
  popUp("To move the selected polygons use Shift-Mouse.");
  return;
}

void polyView::deselectPolysDeleteHlts(){

    size_t cnt = m_highlights.size();
    // a vector of dPoly objects
    m_highlights.clear();

    if (cnt)
        cout << "Cleared " << cnt << " hightlights" << endl;

  markPolysInHlts(m_polyVec, m_highlights, // Inputs
                  m_selectedPolyIndices    // Outputs
                  );
  
  saveDataForUndo(false);
  refreshPixmap();
  return;
}

void polyView::cutToHlt(){
  
  // Cut to the last highlight
  int numH = m_highlights.size();
  if ( numH == 0){
    popUp("No highlights are present. Create one with Control-Mouse.");
    return;
  }
  
  dPoly H = m_highlights[numH - 1];
  assert(H.get_totalNumVerts() == 4);
  double xl, yl, xh, yh;
  H.bdBox(xl, yl, xh, yh);
  printCmd( "clip", xl, yl, xh - xl, yh - yl );
    
  dPoly clippedPoly;
  for (int vecIter = 0; vecIter < (int)m_polyVec.size(); vecIter++){

    m_polyVec[vecIter].clipPoly(xl, yl, xh, yh, //inputs
                                clippedPoly     // output
                                );

    m_polyVec[vecIter] = clippedPoly;    
  }

  m_highlights.resize(numH - 1);
  markPolysInHlts(m_polyVec, m_highlights, // Inputs
                  m_selectedPolyIndices    // Outputs
                  );
  
  saveDataForUndo(false);
  refreshPixmap();

  return;
}

void polyView::enforce45(){
  
  // Enforce that polygon vertices are integers and the angles are 45x. 

  printCmd("enforce45");
    
  for (int vecIter = 0; vecIter < (int)m_polyVec.size(); vecIter++){
    m_polyVec[vecIter].enforce45();
  }

  saveDataForUndo(false);
  refreshPixmap();

  return;
}

void polyView::enforce45AndSnapToGrid(){
  
  // Enforce that polygon vertices are on grid that the angles are 45x. 

  if (!m_prefs.isGridOn){
    popUp("Must have the grid on to snap to grid.");
    return;
  }

  if (m_prefs.gridSize <= 0){
    popUp("Error: expecting positive grid size.");
    return;
  }
  
  for (int vecIter = 0; vecIter < (int)m_polyVec.size(); vecIter++){
    m_polyVec[vecIter].scale(1.0/m_prefs.gridSize);
    m_polyVec[vecIter].enforce45();
    m_polyVec[vecIter].scale(m_prefs.gridSize);
  }

  saveDataForUndo(false);
  refreshPixmap();

  return;
}

void polyView::saveDataForUndo(bool resetViewOnUndo){

  // Save the current geometry and other data. This must
  // be called AFTER each event which changes any data
  // which we would like to undo later.

  // The functions saveDataForUndo and restoreDataAtUndoPos
  // are very intimately related.
  
  m_posInUndoStack++;
  assert(m_posInUndoStack >= 0);

  m_polyVecStack.resize(m_posInUndoStack);
  m_polyVecStack.push_back(m_polyVec);
  
  m_polyOptionsVecStack.resize(m_posInUndoStack);
  m_polyOptionsVecStack.push_back(m_polyOptionsVec);

  m_highlightsStack.resize(m_posInUndoStack);
  m_highlightsStack.push_back(m_highlights);

  m_resetViewStack.resize(m_posInUndoStack);
  m_resetViewStack.push_back(resetViewOnUndo);

  return;
}

void polyView::restoreDataAtUndoPos(){
  
  // The functions saveDataForUndo and restoreDataAtUndoPos
  // are very intimately related.

  assert(m_posInUndoStack >= 0  &&
         m_posInUndoStack < (int)m_polyVecStack.size());
  
  m_polyVec        = m_polyVecStack[m_posInUndoStack];
  m_polyOptionsVec = m_polyOptionsVecStack[m_posInUndoStack];
  m_highlights     = m_highlightsStack[m_posInUndoStack];
  markPolysInHlts(m_polyVec, m_highlights, // Inputs
                  m_selectedPolyIndices    // Outputs
                  );

  return;
}

void polyView::undo(){

  if (m_posInUndoStack <= 0){
    cout << "No actions to undo" << endl;
    return;
  }

  m_posInUndoStack--;
  restoreDataAtUndoPos();

  bool resetViewOnUndo = m_resetViewStack[m_posInUndoStack + 1];
  if (resetViewOnUndo) resetView();
  else                 refreshPixmap();

  return;
}

void polyView::redo(){

  if (m_posInUndoStack + 1 >= (int)m_polyVecStack.size()){
    cout << "No actions to redo" << endl;
    return;
  }

  m_posInUndoStack++;
  restoreDataAtUndoPos();

  bool resetViewOnUndo = m_resetViewStack[m_posInUndoStack];
  if (resetViewOnUndo) resetView();
  else                 refreshPixmap();
  
  return;
}


void polyView::readAllPolys(){

  int numFiles = m_polyOptionsVec.size();
  m_polyVec.resize(numFiles);

  string missingFiles = "";
  int numMissing = 0;
  
  for (int fileIter = 0; fileIter < numFiles; fileIter++){
    
    bool success = readOnePoly(// inputs
                               m_polyOptionsVec[fileIter].polyFileName,
                               m_polyOptionsVec[fileIter].plotAsPoints,
                               m_polyOptionsVec[fileIter].isPolyClosed,
                               // output
                               m_polyVec[fileIter]
                               );
    if (!success){
      missingFiles += " " + m_polyOptionsVec[fileIter].polyFileName;
      numMissing++;
    }
    
    if (m_polyOptionsVec[fileIter].useCmdLineColor){
      m_polyVec[fileIter].set_color(m_polyOptionsVec[fileIter].cmdLineColor);
    }
    
  }


  if (numMissing >= 1){
    string suffix = ""; if (numMissing > 1) suffix = "s";
    popUp("Warning: Could not read file" + suffix + ":" + missingFiles + ".");
  }
  
  saveDataForUndo(false);

  return;
}

void polyView::chooseFilesToShow(){
  m_chooseFilesDlg.chooseFiles(m_polyOptionsVec);
  // User's choice is processed in showFilesChosenByUser().
  return;
}

void polyView::showFilesChosenByUser(){
  
  // Process user's choice from chooseFilesToShow().

  m_filesToHide.clear();
  QTableWidget * filesTable = m_chooseFilesDlg.getFilesTable();
  QList<QTableWidgetItem*> selectedRows = filesTable->selectedItems();
  QListIterator<QTableWidgetItem*> i(selectedRows);
  while (i.hasNext()){
    QTableWidgetItem* item = i.next();
    string fileName = ((item->data(0)).toString()).toStdString();
    m_filesToHide.insert(fileName);
  }
  
  refreshPixmap();
  
  return;
}

/* ------------------------------------------------------------------
   Qt5
   getOpenFileName(
        QWidget *parent = Q_NULLPTR, 
        const QString &caption = QString(), 
        const QString &dir = QString(), 
        const QString &filter = QString(), 
        QString *selectedFilter = Q_NULLPTR, 
        Options options = Options() )

 * ------------------------------------------------------------------ */
static QString caption = ("Choose a file");
static QString filter = ("XG Files (*.xg *.ly* *.pol)");

void polyView::openPoly(){

    //QString dir = QDir::currentDirPath();
#ifdef USE_QT4_DEFS
  QString s = Q3FileDialog::getOpenFileName(QDir::currentDirPath(),
                                           "(*.xg *.ly* *.pol)",
                                           this,
                                           "Open file dialog"
                                           "Choose a file"
                                           );

#else
    // TODO: Improve OPENFILE dialog defaults...
    const char *cwd = getcwd(NULL, 0);
    QString dir = "";
    if (cwd)
        dir = cwd;
    QString s = QFileDialog::getOpenFileName(this, caption, dir, filter);
#endif
  if (s.length() == 0) return;
  
  string fileName = s.toStdString();
  
  assert ( (int)m_polyVec.size() == (int)m_polyOptionsVec.size() );

  m_polyOptionsVec.push_back(m_prefs);
  m_polyOptionsVec.back().polyFileName = fileName;
  
  dPoly poly;
  // poly.reset();
  bool success = readOnePoly(// inputs
                             m_polyOptionsVec.back().polyFileName,
                             m_polyOptionsVec.back().plotAsPoints,
                             m_polyOptionsVec.back().isPolyClosed,
                             // output
                             poly
                             );

  if (!success){
    popUp("Warning: Could not read file: " + m_polyOptionsVec.back().polyFileName + ".");
  }

  if (m_polyOptionsVec.back().useCmdLineColor){
    poly.set_color(m_polyOptionsVec.back().cmdLineColor);
  }
  m_polyVec.push_back(poly);

  saveDataForUndo(false);
  resetView();
  
  return;
}

bool polyView::readOnePoly(// inputs
                           std::string   & filename,
                           bool            plotPointsOnly,
                           closedPolyInfo  isPolyClosed,
                           // output
                           dPoly & poly           
                           ){

  poly.reset();
  
  string type = getFilenameExtension(filename);
  
  if (type == "pol" || type == "cnt"){
     if ( poly.read_pol_or_cnt_format(filename, type, plotPointsOnly) ) return true;
     string msg = string("Invalid .") + type + " format for " + filename
       + ". Trying to read it in .xg format.";
     cerr << msg << endl;
  }
  
  if ( ! poly.readPoly(filename, plotPointsOnly) ){
    return false;
  }

  bool isClosed;
  if (isPolyClosed == forceClosedPoly){
    isClosed = true;
    poly.set_isPolyClosed(isClosed);
  }else if (isPolyClosed == forceNonClosedPoly){
    isClosed = false;
    poly.set_isPolyClosed(isClosed);
  } // else use the isClosed info from file
  
  return true;
}

  
void polyView::saveOnePoly()
{
//#ifdef _MSC_VER // getSaveFileName()
    QString filter = "(*.xg)";
#ifdef USE_QT4_DEFS
    QString s = Q3FileDialog::getSaveFileName("","",this,"Save as one file","Save as one file",&filter);
#else
    QString s = QFileDialog::getSaveFileName(this,  "Save as one file", "savedPoly.xg", "(*.xg)");
#endif
  if (s.length() == 0) return;
  
  string fileName = s.toStdString();

  dPoly poly;

  for (int polyIter = 0; polyIter < (int)m_polyVec.size(); polyIter++){
    poly.appendPolygons(m_polyVec[polyIter]); 
  }

  poly.writePoly(fileName);
  cout << "Polygons saved to " << fileName << endl;

  return;
}

void polyView::overwriteMultiplePolys(){
  bool overwrite = true;
  writeMultiplePolys(overwrite);
}
#ifdef ADD_IMAGE_SAVE
// Do I have to do the scaling, like
// pixmap.scaled(32, 32, Qt::IgnoreAspectRatio, Qt::FastTransformation));
// hmmm, it seems NOT, the pixel map is already 'sized'
// WOW, it really seems all that simple ;=))
void polyView::saveScreenImage()
{
    const QPixmap *img = &m_pixmap;
    QString saveFilename = QFileDialog::getSaveFileName(this, "Save as", "Choose a filename", "PNG(*.png);; TIFF(*.tiff *.tif);; JPEG(*.jpg *.jpeg)");
    if (saveFilename.length() == 0)
        return;
    QString saveExtension = "PNG";
    int pos = saveFilename.lastIndexOf('.');
    if (pos >= 0)
        saveExtension = saveFilename.mid(pos + 1);
    else 
        saveFilename += ".png";

#ifdef USE_QT4_DEFS
    int res = img->save(saveFilename,saveExtension);
#else
    int res = img->save(saveFilename, saveExtension.toStdString().c_str());
#endif
    if (!res) {
        QMessageBox::warning(this, "File could not be saved!", "ok", QMessageBox::Ok);
    }

}
#endif // ADD_IMAGE_SAVE


void polyView::saveAsMultiplePolys(){
  bool overwrite = false;
  writeMultiplePolys(overwrite);
}

void polyView::writeMultiplePolys(bool overwrite){

  string allFiles = "";
  for (int polyIter = 0; polyIter < (int)m_polyVec.size(); polyIter++){

    dPoly poly = m_polyVec[polyIter];
    
    string fileName = m_polyOptionsVec[polyIter].polyFileName;
    if (!overwrite) fileName = inFileToOutFile(fileName);

    poly.writePoly(fileName.c_str());
    allFiles += " " + fileName;
  }

  if ((int)m_polyVec.size() > 0){
    cout << "Polygons saved to" << allFiles << endl;
  }

  return;
}

void polyView::togglePE(){

  m_toggleShowPointsEdges = m_toggleShowPointsEdges%3 + 1;
  refreshPixmap();
  
}

void polyView::changeOrder(){

  m_changeDisplayOrder = true;
  refreshPixmap();
  
}

#ifdef USE_QT4_DEFS // ref Q3PointArray
bool polyView::isPolyZeroDim(const Q3PointArray & pa){

  int numPts = pa.size();
  for (int s = 1; s < numPts; s++){
    if (pa[0] != pa[s]) return false;
  }
  
  return true;
}
#else 
bool polyView::isPolyZeroDim(const QPolygon & pa){

  int numPts = pa.size();
  for (int s = 1; s < numPts; s++){
    if (pa[0] != pa[s]) return false;
  }
  
  return true;
}

#endif
void polyView::initTextOnScreenGrid(std::vector< std::vector<int> > & Grid){

  // Split the screen into numGridPts x numGridPts rectangles.  For
  // performance reasons, will not allow more than one string of text
  // do be displayed in one rectangle. As such, if a polygon has a lot
  // of text (annotations), the more you zoom in the more of the
  // annotations you will see.
  int numGridPts = 20; 
  
  Grid.resize(numGridPts);
  for (int s = 0; s < (int)Grid.size(); s++){
    Grid[s].resize(numGridPts);
    for (int t = 0; t < (int)Grid[s].size(); t++){
      Grid[s][t] = 0; // All points start free
    }
  }
}

bool polyView::isClosestGridPtFree(std::vector< std::vector<int> > & Grid,
                                   int x, int y){

  int numGridPts = Grid.size();
  if (numGridPts <= 0) return false;
  
  // Take a point on the screen and snap it to the closest grid point
  int sx = (int)round ((numGridPts - 1)*(double(x - m_screenXll)/double(m_screenWidX)));
  sx = max(sx, 0); sx = min(sx, numGridPts - 1);
  
  int sy = (int)round ((numGridPts - 1)*(double(y - m_screenYll)/double(m_screenWidY)));
  sy = max(sy, 0); sy = min(sy, numGridPts - 1);

  int maxAllow = 2; // Allow at most this many text labels around one grid point
  if (Grid[sx][sy] <= maxAllow - 1){
    Grid[sx][sy]++;
    return true;
  }else{
    return false;
  }
  
  return false;
}

double polyView::pixelToWorldDist(int pd){

  double x0, x1, y0, y1;
  pixelToWorldCoords(0,  0, x0, y0);
  pixelToWorldCoords(pd, 0, x1, y1);

  return abs(x0 - x1);
  
}

void polyView::setStandardCursor(){
  QCursor C(Qt::ArrowCursor);
  setCursor(C);
}

void polyView::setPolyDrawCursor(){
  QCursor C(Qt::CrossCursor);
  setCursor(C);
}

void polyView::setupDisplayOrder(// Inputs
                                 int                 numPolys,
                                 // Input-output
                                 bool              & changeDisplayOrder,
                                 std::vector<int>  & polyVecOrder
                                 ){


  // Decide the order in which polygons are displayed. 

  if ((int)polyVecOrder.size() != numPolys){

    // Default order
    polyVecOrder.resize(numPolys);
    for (int c = 0; c < numPolys; c++){
      polyVecOrder[c] = c;
    }

  }else if (changeDisplayOrder && numPolys >= 1){

    changeDisplayOrder = false;

    // Cycle left
    int bk = polyVecOrder[0];
    for (int c = 1; c < numPolys; c++) polyVecOrder[c-1] = polyVecOrder[c];
    polyVecOrder[numPolys - 1] = bk;
    
    
  }

  return;
}

void polyView::printCmd(std::string cmd, const std::vector<double> & vals){

  ostringstream S;
  int prec = 16;
  S.precision(prec);
  S << cmd;
  for (int p = 0; p < (int)vals.size(); p++){
    S << ' ' << vals[p];
  }
  S << endl;
  
  cout << S.str();

  return;
}

void polyView::printCmd(std::string cmd, double xll, double yll,
                        double widX, double widY){

  ostringstream S;
  int prec = 16;
  S.precision(prec);
  S << cmd << ' ' << xll << ' ' << yll << ' ' << widX << ' ' << widY << endl;
  cout << S.str();

  return;
}
                                               
void polyView::printCmd(std::string cmd){

  ostringstream S;
  int prec = 16;
  S.precision(prec);
  S << cmd << endl;
  cout << S.str();

  return;
}

void polyView::runCmd(std::string cmd){

  string cmdName = "";
  vector<double> vals; vals.clear();
  double val;
  
  istringstream in(cmd);
  if (in >> cmdName){

    while (in >> val) vals.push_back(val);

    // Commands with no arguments
    if (cmdName == "enforce45"){ enforce45();          return; }
    if (cmdName == "poly_diff"){ toggleShowPolyDiff(); return; }
  
    // Process a command with four numbers are input arguments
    if (cmdName == "view"){

      if (vals.size() >= 4){
        double xll = vals[0], yll = vals[1], widx = vals[2], widy = vals[3];
        if (xll + widx > xll && yll + widy > yll){
          m_viewXll = xll; m_viewWidX = widx;
          m_viewYll = yll; m_viewWidY = widy;
          m_viewChanged = true;
          refreshPixmap();
          return;
        }
      }
      cerr << "Invalid view command: " << cmd << endl;
      return;
      
    }else if (cmdName == "clip"){
      
      if (vals.size() >= 4){
        double xll = vals[0], yll = vals[1], widx = vals[2], widy = vals[3];
        if (xll + widx > xll && yll + widy > yll){
          createHighlightWithRealInputs(xll, yll, xll + widx, yll + widy);
          cutToHlt();
          return;
        }
      }
      cerr << "Invalid clip command: " << cmd << endl;
      return;

    }else if (cmdName == "erasePolysInHlt"){
      
      if (vals.size() >= 4){
        double xll = vals[0], yll = vals[1], widx = vals[2], widy = vals[3];
        if (xll + widx > xll && yll + widy > yll){
          createHighlightWithRealInputs(xll, yll, xll + widx, yll + widy);
          deleteSelectedPolys();
          return;
        }
      }
      cerr << "Invalid command: " << cmd << endl;
      return;
      
    }else if (cmdName == "translate"){
      if (vals.size() >= 2){
        translatePolys(vals);
        return;
      }
      cerr << "Invalid translate command: " << cmd << endl;
      return;
      
    }else if (cmdName == "rotate"){
      if (vals.size() >= 1){
        rotatePolys(vals);
        return;
      }
      cerr << "Invalid rotate command: " << cmd << endl;
      return;
    }else if (cmdName == "scale"){
      if (vals.size() >= 1){
        scalePolys(vals);
        return;
      }
      cerr << "Invalid scale command: " << cmd << endl;
      return;
    }else if (cmdName == "transform"){
      if (vals.size() >= 6){
        transformPolys(vals);
        return;
      }
      cerr << "Invalid transform command: " << cmd << endl;
      return;
    }else if (cmdName == "translate_selected"){
      if (vals.size() >= 2){
        translateSelectedPolys(vals);
        return;
      }
      cerr << "Invalid translate command: " << cmd << endl;
      return;
    }else if (cmdName == "rotate_selected"){
      if (vals.size() >= 1){
        rotateSelectedPolys(vals);
        return;
      }
      cerr << "Invalid rotate command: " << cmd << endl;
      return;
    }else if (cmdName == "scale_selected"){
      if (vals.size() >= 1){
        scaleSelectedPolys(vals);
        return;
      }
      cerr << "Invalid scale command: " << cmd << endl;
      return;
    }else if (cmdName == "transform_selected"){
      if (vals.size() >= 4){
        transformSelectedPolys(vals);
        return;
      }
      cerr << "Invalid transform command: " << cmd << endl;
      return;
    }else if (cmdName == "mark"){
      if (vals.size() >= 2){
        plotMark(vals[0], vals[1]);
        return;
      }
      cerr << "Invalid mark command: " << cmd << endl;
      return;
    }
    
  }
  if ((cmdName == "-h" )|| (cmdName == "--help" )||
      (cmdName == "-?" )|| (cmdName == "?")) 
  {
      cerr << "Valid commands:" << endl;
      cerr << "Single: 'enforce45' 'poly_diff'" << endl;
      cerr << "with 2 inputs: 'mark' xll yll" << endl;
      cerr << "With 4 inputs: 'view' 'clip' 'erasePolysInHlt' xll yll widx widy" << endl; 
      cerr << "Others: 'translate' shift_x shift_y ..." << endl;
      cerr << "'translate_selected' shift_x shift_y" << endl;
      cerr << "'rotate_selected' angle" << endl;
      cerr << "'scale_selected' scale" << endl;
      cerr << "'transform_selected' 4 matrix" << endl;
  } else {
      cerr << "Invalid command: " << cmd << endl;
  }
  
  return;
}

double polyView::calcGrid(double widx, double widy){

  double grid = max(widx, widy)/50;

  if (grid <= 0){
    cout << "Warning: non-positive width and height" << endl;
    return 1.0;
  }

  // Values bigger than 1 are snapped to grid of 1, bigger than 2^n
  // to grid of 2^n.
  int k = (int)round( log(grid)/log(2.0) );
  double v;
  if (k >= 0) v = round( pow(2.0, k) );
  else        v = 1.0/round( pow(2.0, -k) );
  grid = v*round(grid/v);

  return grid;
}
  
void polyView::mergePolys(){

  // Merge all polygons.
  
  // Highly buggy and incomplete function. Work in progress.
  // To do: Move this to utilities.
  
  // Combine all polygons into one poly structure
  dPoly poly;
  poly.reset();
  for (int s = 0; s < (int)m_polyVec.size(); s++) poly.appendPolygons(m_polyVec[s]);

  bool stillMerging = true;

  while (stillMerging){

    stillMerging = false;
    
    // Pairwise merge of polygons
    for (int i = 0; i < poly.get_numPolys(); i++){
      for (int j = i + 1; j < poly.get_numPolys(); j++){

        // Need these since the polygons may have changed in the meantime
        if (i >= poly.get_numPolys()) break;
        if (j >= poly.get_numPolys()) break;

        // Merge i-th and j-th polygons
        const int * numV = poly.get_numVerts();
        int start_i = 0; for (int pIter = 0; pIter < i; pIter++) start_i += numV[pIter]; 
        int start_j = 0; for (int pIter = 0; pIter < j; pIter++) start_j += numV[pIter];
        vector<double> mx, my;
        bool success = utils::mergePolys(// Inputs
                                         numV[i],
                                         poly.get_xv() + start_i,
                                         poly.get_yv() + start_i,  
                                         numV[j],
                                         poly.get_xv() + start_j,
                                         poly.get_yv() + start_j,
                                         // Outputs
                                         mx, my
                                         );
        if (!success) continue;

        stillMerging = true;
        
        // Replace i-th poly with the merged poly, and erase j-th poly.
        // Decrement j since we have one less polygon.
        poly.replaceOnePoly(i, mx.size(), vecPtr(mx), vecPtr(my));
        poly.eraseOnePoly(j);
        j--;
      }
    }
    
  }
  
  if (m_polyVec.empty()) return;
  m_polyVec.resize(1);
  m_polyOptionsVec.resize(1);
  m_polyVec[0] = poly; 
  saveDataForUndo(false);

  refreshPixmap();
  return;
}  

void polyView::deleteSelectedPolys(){
  
  eraseMarkedPolys(// Inputs
                   m_selectedPolyIndices,  
                   // Inputs-outputs
                   m_polyVec
                   );
  
  m_highlights.clear();
  
  markPolysInHlts(m_polyVec, m_highlights, // Inputs
                  m_selectedPolyIndices    // Outputs
                  );
  
  saveDataForUndo(false);
  refreshPixmap();

  return;
}

#ifdef ADD_PREF_INI
void polyView::writeINI()
{
    polyOptions *popt = &m_prefs;
    QString s;
    setINIFile();
    // writeSetting("INIFile", m_sSettingsFile);
    writeSetting("fontSize",popt->fontSize);
    writeSetting("lineWidth",popt->lineWidth);
    writeSetting("isGridOn",popt->isGridOn);
    writeSetting("gridSize",popt->gridSize);
    writeSetting("gridWidth",popt->gridWidth);
    writeSetting("isLatLonOn",popt->isLatLonOn);
    s = QString::fromStdString(popt->bgColor);
    writeSetting("bgColor",s);
    s = QString::fromStdString(popt->fgColor);
    writeSetting("fgColor",s);
    s = QString::fromStdString(popt->cmdLineColor);
    writeSetting("cmdLineColor",s);
    s = QString::fromStdString(popt->gridColor);
    writeSetting("gridColor",s);
    s = QString::fromStdString(popt->latlonColor);
    writeSetting("latlonColor",s);
    s = QString::fromStdString(popt->polyFileName);
    writeSetting("polyFileName",s);

    writeSetting("useNmScale", m_useNmScale);
}

void polyView::readINI()
{
    //polyOptions *popt = &m_prefs;
    //QString s;
    //setINIFile();
    // QSettings settings(m_sSettingsFile, QSettings::IniFormat);
    // proceceed to read settings...
    // or use a utility functions to do the same...
    m_showAnnotations    = utils::loadSettingBool("m_showAnnotations",m_showAnnotations);
    m_showVertIndexAnno  = utils::loadSettingBool("m_showVertIndexAnno",m_showVertIndexAnno);
    m_showLayerAnno      = utils::loadSettingBool("m_showLayerAnno",m_showLayerAnno);
    m_useNmScale         = utils::loadSettingBool("useNmScale",false);

}
#endif // ADD_PREF_INI

