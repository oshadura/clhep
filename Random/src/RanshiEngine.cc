// $Id: RanshiEngine.cc,v 1.1.1.1 2003/07/15 20:15:05 garren Exp $
// -*- C++ -*-
//
// -----------------------------------------------------------------------
//                           HEP Random
//                      --- RanshiEngine ---
//                    class implementation file
// -----------------------------------------------------------------------
//
// This algorithm implements the random number generator as proposed by 
// "F. Gutbrod, Comp. Phys. Comm. 87 (1995) 291-306". 
//
// =======================================================================
// Ken Smith      - Created:                                 9th June 1998
//                - Removed pow() from flat method:          21st Jul 1998
//                - Added conversion operators:               6th Aug 1998
// J. Marraffino  - Added some explicit casts to deal with
//                  machines where sizeof(int) != sizeof(long) 22 Aug 1998
// M. Fischler    - Modified constructors taking seeds to not
//                  depend on numEngines (same seeds should
//                  produce same sequences).  Default still
//                  depends on numEngines.                      16 Sep 1998
//                - Modified use of the various exponents of 2
//                  to avoid per-instance space overhead and
//                  correct the rounding procedure              16 Sep 1998
// J. Marraffino  - Remove dependence on hepString class        13 May 1999
//
// =======================================================================

#include "CLHEP/Random/RanshiEngine.h"
#include <string.h>
#include <cmath>	// for ldexp()

using namespace std;

static const int MarkerLen = 64; // Enough room to hold a begin or end marker. 

double RanshiEngine::twoToMinus_32;
double RanshiEngine::twoToMinus_53;
double RanshiEngine::nearlyTwoToMinus_54;

void RanshiEngine::powersOfTwo() {
  twoToMinus_32 = ldexp (1.0, -32);
  twoToMinus_53 = ldexp (1.0, -53);
  nearlyTwoToMinus_54 = ldexp (1.0, -54) - ldexp (1.0, -100);
}

// Number of instances with automatic seed selection
int RanshiEngine::numEngines = 0;

RanshiEngine::RanshiEngine() : halfBuff(0), numFlats(0) 
{
  powersOfTwo();
  int i = 0;
  while (i < numBuff) {    
    buffer[i] = (unsigned int)(numEngines+19780503L*(i+1));
    ++i;
  }
  theSeed = numEngines+19780503L*++i;
  redSpin = (unsigned int)(theSeed & 0xffffffff);
  ++numEngines;
  for( i = 0; i < 10000; ++i) flat();  // Warm-up by running thorugh 10000 nums
}

RanshiEngine::RanshiEngine(std::istream& is) : halfBuff(0), numFlats(0) 
{
  is >> *this;
}

RanshiEngine::RanshiEngine(long seed) : halfBuff(0), numFlats(0) 
{
  powersOfTwo();
  for (int i = 0; i < numBuff; ++i) {
    buffer[i] = (unsigned int)seed&0xffffffff;
  }
  theSeed = seed;
  redSpin = (unsigned int)(theSeed & 0xffffffff);
  int j;
  for (j = 0; j < numBuff*20; ++j) {      // "warm-up" for engine to hit
    flat();                               //  every ball on average 20X.
  }
}

RanshiEngine::RanshiEngine(int rowIndex, int colIndex)
: halfBuff(0), numFlats(0) 
{
  powersOfTwo();
  int i = 0;
  while( i < numBuff ) {
    buffer[i] = (unsigned int)((rowIndex + (i+1)*(colIndex+8))&0xffffffff);
    ++i;
  }
  theSeed = rowIndex;
  redSpin = colIndex & 0xffffffff;
  for( i = 0; i < 100; ++i) flat();    // Warm-up by running thorugh 100 nums
}

RanshiEngine::~RanshiEngine() { }

RanshiEngine::RanshiEngine( const RanshiEngine & p ) 
: halfBuff(0), numFlats(0) 
{
  *this = p;
}

RanshiEngine & RanshiEngine::operator=( const RanshiEngine & p ) {
  if( this != &p ) {
    halfBuff = p.halfBuff;
    numFlats = p.numFlats;
    redSpin  = p.redSpin;
    for( int i =0; i < numBuff; ++i) {
      buffer[i] = p.buffer[i];
    }
  }
  return *this;
}

double RanshiEngine::flat() {
  unsigned int redAngle = (((numBuff/2) - 1) & redSpin) + halfBuff;
  unsigned int blkSpin     = buffer[redAngle] & 0xffffffff;
  unsigned int boostResult = blkSpin ^ redSpin;

  buffer[redAngle] = ((blkSpin << 17) | (blkSpin >> (32-17))) ^ redSpin;
  
  redSpin  = (blkSpin + numFlats++) & 0xffffffff;
  halfBuff = numBuff/2 - halfBuff;
  
  return ( blkSpin * twoToMinus_32 +            // most significant part
	   (boostResult>>11) * twoToMinus_53 +  // fill in remaining bits
	   nearlyTwoToMinus_54);  		// non-zero
}

void RanshiEngine::flatArray(const int size, double* vect) {
  for (int i = 0; i < size; ++i) {
    vect[i] = flat();
  }
}

void RanshiEngine::setSeed(long seed, int) {
  *this = RanshiEngine(seed); 
}

void RanshiEngine::setSeeds(const long* seeds, int) {
  if (*seeds) {
    int i = 0;
    while (seeds[i] && i < numBuff) {
      buffer[i] = (unsigned int)seeds[i];
      ++i;
    }
    while (i < numBuff) {
      buffer[i] = buffer[i-1];
      ++i;
    }
    theSeed = seeds[0];
    redSpin = (unsigned int)theSeed;
  }
  theSeeds = seeds;
}
     
void RanshiEngine::saveStatus(const char filename[]) const {
  std::ofstream outFile(filename, std::ios::out);
  if (!outFile.bad()) {
    outFile << std::setprecision(20) << theSeed << std::endl;
    for (int i = 0; i < numBuff; ++i) {
      outFile << buffer[i] << " ";
    }
    outFile << redSpin  << " " << numFlats << " " << halfBuff << std::endl;
  }
}

void RanshiEngine::restoreStatus(const char filename[]) {
  std::ifstream inFile(filename, std::ios::in);
  if (!inFile.bad()) {
    inFile >> theSeed;
    for (int i = 0; i < numBuff; ++i) {
      inFile >> buffer[i];
    }
    inFile >> redSpin >> numFlats >> halfBuff;
  }
}

void RanshiEngine::showStatus() const {
  std::cout << std::setprecision(20) << std::endl;
  std::cout << "----------- Ranshi engine status ----------" << std::endl;
  std::cout << "Initial seed      = " << theSeed << std::endl;
  std::cout << "Current red spin  = " << redSpin << std::endl;
  std::cout << "Values produced   = " << numFlats << std::endl;
  std::cout << "Side of buffer    = " << (halfBuff ? "upper" : "lower")
	    << std::endl;
  std::cout << "Current buffer    = " << std::endl;
  for (int i = 0; i < numBuff; i+=4) {
    std::cout << std::setw(10) << std::setiosflags(std::ios::right)
	      << buffer[i]     << std::setw(11) << buffer[i+1] << std::setw(11)
	      << buffer[i+2]   << std::setw(11) << buffer[i+3] << std::endl;
  }
  std::cout << "-------------------------------------------" << std::endl;
}

RanshiEngine::operator float() {
  unsigned int redAngle = (((numBuff/2) - 1) & redSpin) + halfBuff;
  unsigned int blkSpin  = buffer[redAngle] & 0xffffffff;
  
  buffer[redAngle] = ((blkSpin << 17) | (blkSpin >> (32-17))) ^ redSpin;
  
  redSpin  = (blkSpin + numFlats++) & 0xffffffff;
  halfBuff = numBuff/2 - halfBuff;
  
  return float(blkSpin * twoToMinus_32);
}

RanshiEngine::operator unsigned int() {
  unsigned int redAngle = (((numBuff/2) - 1) & redSpin) + halfBuff;
  unsigned int blkSpin  = buffer[redAngle] & 0xffffffff;
  
  buffer[redAngle] = ((blkSpin << 17) | (blkSpin >> (32-17))) ^ redSpin;
  
  redSpin  = (blkSpin + numFlats++) & 0xffffffff;
  halfBuff = numBuff/2 - halfBuff;
  
  return blkSpin;
}

std::ostream& operator<< (std::ostream& os, const RanshiEngine& e) {
  char beginMarker[] = "RanshiEngine-begin";
  char endMarker[]   = "RanshiEngine-end";
 
  os << " " << beginMarker << " ";
  
  os << std::setprecision(20)  << e.theSeed  << " ";
  for (int i = 0; i < e.numBuff; ++i) {
    os << e.buffer[i]  << " ";
  }
  os << e.redSpin  << " " << e.numFlats << " " << e.halfBuff; 
  
  os << " " << endMarker   << " ";
  return os;
}

std::istream& operator>> (std::istream& is, RanshiEngine& e) {
  char beginMarker [MarkerLen];
  char endMarker   [MarkerLen];

  is >> std::ws;
  is.width(MarkerLen);  // causes the next read to the char* to be <=
			// that many bytes, INCLUDING A TERMINATION \0 
			// (Stroustrup, section 21.3.2)
  is >> beginMarker;
  if (strcmp(beginMarker,"RanshiEngine-begin")) {
    is.clear(std::ios::badbit | is.rdstate());
    std::cerr << "\nInput mispositioned or"
	      << "\nRanshiEngine state description missing or"
	      << "\nwrong engine type found." << std::endl;
    return is;
  }
  is >> e.theSeed;
  for (int i = 0; i < e.numBuff; ++i) {
    is >> e.buffer[i];
  }
  is >> e.redSpin >> e.numFlats >> e.halfBuff;
  is >> std::ws;
  is.width(MarkerLen);  
  is >> endMarker;
  if (strcmp(endMarker,"RanshiEngine-end")) {
    is.clear(std::ios::badbit | is.rdstate());
    std::cerr << "\nRanshiEngine state description incomplete."
	      << "\nInput stream is probably mispositioned now." << std::endl;
    return is;
  }
  return is;
}