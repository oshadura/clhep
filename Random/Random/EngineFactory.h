// $Id 
// -*- C++ -*-
//
// -----------------------------------------------------------------------
//                             HEP Random
//                       --- EngineFactory ---
//                          class header file
// -----------------------------------------------------------------------

// Class generating new engines from streamed saves.

// =======================================================================
// M Fischler     - Created: not yet really
// =======================================================================

#ifndef EngineFactory_h
#define EngineFactory_h 1

#include "CLHEP/Random/defs.h"
#include "CLHEP/Random/RandomEngine.h"

namespace CLHEP {

class EngineFactory {
public:
  static HepRandomEngine* newEngine(std::istream & is);
};


}  // namespace CLHEP

#ifdef ENABLE_BACKWARDS_COMPATIBILITY
//  backwards compatibility will be enabled ONLY in CLHEP 1.9
using namespace CLHEP;
#endif


#endif

