# Build
TEMPLATE        = app
TARGET          = polyview
QT             += qt3support
RESOURCES     = polyview.qrc
INCLUDEPATH += geom
SOURCES = gui/mainProg.cpp gui/polyView.cpp gui/appWindow.cpp gui/chooseFilesDlg.cpp gui/utils.cpp gui/documentation.cpp geom/dPoly.cpp geom/cutPoly.cpp geom/geomUtils.cpp geom/polyUtils.cpp geom/edgeUtils.cpp geom/dTree.cpp geom/kdTree.cpp
HEADERS = gui/polyView.h gui/appWindow.h gui/chooseFilesDlg.h gui/utils.h geom/dPoly.h geom/cutPoly.h  geom/geomUtils.h geom/polyUtils.h geom/edgeUtils.h geom/dTree.h geom/kdTree.h geom/baseUtils.h

# Install
polyview.path = $$(INSTALL_DIR)/bin # install directory

win32 {
  polyview.files = polyview
}
unix {
  polyview.files = polyview
}
macx {
  polyview.files =  polyview.app/Contents/MacOS/polyview
}

INSTALLS += polyview
