// $Id: RandPoissonT.h,v 1.1.1.1 2003/07/15 20:15:05 garren Exp $
// -*- C++ -*-
//
// -----------------------------------------------------------------------
//                             HEP Random
//                         --- RandPoissonT ---
//                          class header file
// -----------------------------------------------------------------------

// Class defining methods for shooting numbers according to the Poisson
// distribution, given a mean.  RandPoissonT is derived from RandPoisson
// and shares the identical user interface.  RandPoissonT is always 
// perfectly accurate for any value of mu.

// For mu > 100 the algorithm used is taken from the base class RandPoisson
// (Algorithm from "W.H.Press et al., Numerical Recipes in C, Second Edition".)
//
// For mu < 100, algorithm used is a table lookup based on [mu/K] for some 
// smallish K, followed by an explicit series-drived poisson for the small 
// remaining part of mu.  This method is exact, and is substantially faster 
// than the method used by the base class.  The implementation of this method 
// is in the RandPoissonQ class.

// =======================================================================
// M. Fischler    - Created 26 Jan 2000
// =======================================================================

#ifndef RandPoissonT_h
#define RandPoissonT_h 1

#include "CLHEP/Random/RandPoisson.h"

/**
 * @author
 * @ingroup random
 */
class RandPoissonT : public RandPoisson {

public:

  RandPoissonT ( HepRandomEngine& anEngine, double m=1.0 );
  RandPoissonT ( HepRandomEngine* anEngine, double m=1.0 );
  // These constructors should be used to instantiate a RandPoissonT
  // distribution object defining a local engine for it.
  // The static generator will be skipped using the non-static methods
  // defined below.
  // If the engine is passed by pointer the corresponding engine object
  // will be deleted by the RandPoissonT destructor.
  // If the engine is passed by reference the corresponding engine object
  // will not be deleted by the RandPoissonT destructor.

  virtual ~RandPoissonT();
  // Destructor

  // Static methods to shoot random values using the static generator

  static  long shoot( double m=1.0 );

  static  void shootArray ( const int size, long* vect, double m=1.0 );

  //  Static methods to shoot random values using a given engine
  //  by-passing the static generator.

  static  long shoot( HepRandomEngine* anEngine, double m=1.0 );

  static  void shootArray ( HepRandomEngine* anEngine,
                            const int size, long* vect, double m=1.0 );

  //  Methods using the localEngine to shoot random values, by-passing
  //  the static generator.

  long  fire();
  long  fire( double m );

  void fireArray ( const int size, long* vect );
  void fireArray ( const int size, long* vect, double m);

  double operator()();
  double operator()( double m );

private:

  // Private copy constructor. Defining it here disallows use.
  RandPoissonT(const RandPoissonT& d);

};

#include "CLHEP/Random/RandPoissonT.icc"

#endif