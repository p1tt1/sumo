/****************************************************************************/
/// @file    NIVissimSingleTypeParser_Haltestellendefinition.cpp
/// @author  Daniel Krajzewicz
/// @date    Wed, 18 Dec 2002
/// @version $Id: $
///
//
/****************************************************************************/
// SUMO, Simulation of Urban MObility; see http://sumo.sourceforge.net/
// copyright : (C) 2001-2007
//  by DLR (http://www.dlr.de/) and ZAIK (http://www.zaik.uni-koeln.de/AFS)
/****************************************************************************/
//
//   This program is free software; you can redistribute it and/or modify
//   it under the terms of the GNU General Public License as published by
//   the Free Software Foundation; either version 2 of the License, or
//   (at your option) any later version.
//
/****************************************************************************/
// ===========================================================================
// compiler pragmas
// ===========================================================================
#ifdef _MSC_VER
#pragma warning(disable: 4786)
#endif


// ===========================================================================
// included modules
// ===========================================================================
#ifdef WIN32
#include <windows_config.h>
#else
#include <config.h>
#endif

#include <iostream>
#include <utils/common/TplConvert.h>
#include "../NIVissimLoader.h"
#include "../tempstructs/NIVissimClosures.h"
#include "NIVissimSingleTypeParser_Haltestellendefinition.h"

#ifdef _DEBUG
#include <utils/dev/debug_new.h>
#endif // _DEBUG


// ===========================================================================
// used namespaces
// ===========================================================================
using namespace std;


// ===========================================================================
// method definitions
// ===========================================================================
NIVissimSingleTypeParser_Haltestellendefinition::NIVissimSingleTypeParser_Haltestellendefinition(NIVissimLoader &parent)
        : NIVissimLoader::VissimSingleTypeParser(parent)
{}


NIVissimSingleTypeParser_Haltestellendefinition::~NIVissimSingleTypeParser_Haltestellendefinition()
{}


bool
NIVissimSingleTypeParser_Haltestellendefinition::parse(std::istream &from)
{
    string tag;
    while (tag!="strecke") {
        tag = myRead(from);
    }
    while (tag!="laenge") {
        tag = myRead(from);
    }
    from >> tag;
    tag = readEndSecure(from);
    while (tag=="einsteiger"&&tag!="DATAEND") {
        while (tag!="linie") {
            tag = myRead(from);
        }
        while (tag!="einsteiger"&&tag!="DATAEND") {
            tag = readEndSecure(from);
        }
    }
    return true;
}



/****************************************************************************/

