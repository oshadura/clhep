// $Id: ReadNextHepMC.cc,v 1.1.1.1 2003/07/15 20:15:05 garren Exp $
// ----------------------------------------------------------------------
//
// ReadNextHepMC.cc
// Author: Lynn Garren
//
// read from an ascii file
// ----------------------------------------------------------------------

#include "HepMC/defs.h"
#include <iostream>
#include <fstream>
#include <string>

#include "CLHEP/HepMC/ReadHepMC.h"
#include "CLHEP/HepMC/GenEvent.h"

namespace HepMC  {

GenEvent * findNextGenEvent( std::istream & is )
{
    // check the state of the input stream
    if ( !is ) return NULL;
    //
    // find an event block
    std::string type("GenEvent");
    bool ok = findBlockType( is, type );
    // read the event
    if( ok ) {
	GenEvent * evt = readGenEvent( is );
        return evt;
    }
    // no event has been found
    return NULL;
}

}	// HepMC