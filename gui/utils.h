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
#ifndef UTILS_H
#define UTILS_H
#ifdef ADD_PREF_INI
#include <qstring.h>
#include <qvariant.h>
#endif // ADD_PREF_INI
#include <cmath>
#include <iostream>
#include <string>
#include <vector>
#include "../geom/polyUtils.h"

enum closedPolyInfo{
  // If an array of points as read from file has the first vertex equal to the last
  // one, we treat it is a closed polygon (last vertex joins the first vertex).
  // If the user wants to override this behavior, the first two fields below
  // become necessary.
  forceClosedPoly, forceNonClosedPoly, readClosedPolyInfoFromFile
};
  

class polyOptions 
{
public:
  // Each polygon file has these options
  bool            plotAsPoints;
  bool            isPolyFilled;
  closedPolyInfo  isPolyClosed;
  bool            useCmdLineColor;
  int             fontSize;
  int             lineWidth;
  bool            isGridOn;
  double          gridSize;
  int             gridWidth;
  bool            isLatLonOn;   // TODO: Add lat lon 'grid' - saved/rest, but UNUSED
  std::string     bgColor;
  std::string     fgColor;
  std::string     cmdLineColor;
  std::string     gridColor;
  std::string     latlonColor;  // TODO: Add lat lon 'grid' - saved/rest, but UNUSED
  std::string     polyFileName;
  
  polyOptions(){
      // set DEFAULTS
    plotAsPoints    = false;
    isPolyFilled    = false;
    isPolyClosed    = readClosedPolyInfoFromFile;
    fontSize        = 10; 
    lineWidth       = 1;
    useCmdLineColor = false;
    isGridOn        = false;
    isLatLonOn      = false; 
    gridWidth       = 1;
    gridSize        = -1;
    bgColor         = "black";
    fgColor         = "white";
    cmdLineColor    = "green";
    gridColor       = "white";
    latlonColor     = "white";
    polyFileName    = "unnamed.xg";
  }
  
};

class cmdLineOptions
{
public:
  std::vector<polyOptions> polyOptionsVec;
};

namespace utils{

  std::string getDocText();
  
  void extractWindowDims(// inputs
                         int numArgs, char ** args,
                         // outputs
                         int & windowWidX, int & windowWidY
                         );

  void parseCmdOptions(//inputs
                       int argc, char** argv, std::string exeName,
                       // outputs
                       int & windowWidX, int & windowWidY, cmdLineOptions & options
                       );

  std::string inFileToOutFile(const std::string & inFile);

  void printUsage(std::string progName);

  std::string getFilenameExtension(std::string filename);
  std::string replaceAll(std::string result, 
                         const std::string & replaceWhat, 
                         const std::string & replaceWithWhat);
#ifdef ADD_PREF_INI
    void setINIFile();
    void loadSettings(polyOptions &opt, int & windowWidX, int & windowWidY);
    int loadSettingInt(const QString &key, int def);
    std::string loadSettingStdString(const QString &key, std::string def);
    bool loadSettingBool(const QString &key, bool def );
    void writeSetting(const QString &key, const QVariant &variant);
#endif // ADD_PREF_INI

}

// Uses haversine formula to compute great circle distance 
// between (assumed) wsg84 lon lat coordinates. TODO: Toggle m_useNmScale
extern double DistanceInMeters(double lat1, double lon1, double lat2, double lon2);

#endif
