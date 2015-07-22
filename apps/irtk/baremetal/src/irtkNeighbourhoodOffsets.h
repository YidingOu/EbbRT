/*=========================================================================

  Library   : Image Registration Toolkit (IRTK)
  Module    : $Id: irtkNeighbourhoodOffsets.h 747 2013-01-10 11:05:14Z wbai $
  Copyright : Imperial College, Department of Computing
  Visual Information Processing (VIP), 2008 onwards
  Date      : $Date: 2013-01-10 11:05:14 +0000 (Thu, 10 Jan 2013) $
  Version   : $Revision: 747 $
  Changes   : $Author: wbai $

  =========================================================================*/

#ifndef IRTKNEIGHBOURHOODOFFSETS_H_
#define IRTKNEIGHBOURHOODOFFSETS_H_

#include "irtkBaseImage.h"

typedef enum {CONNECTIVITY_04,
	      CONNECTIVITY_06,
	      CONNECTIVITY_18,
	      CONNECTIVITY_26
} irtkConnectivityType;

class irtkNeighbourhoodOffsets : public irtkObject
{

  // Set to maximum possible size.
  int _Offsets[26];

  int _Size;

  irtkConnectivityType _Connectivity;

public:
  //
  // Constructors and destructor
  //

  /// Constructor
  irtkNeighbourhoodOffsets();

  /// Constructor with image and connectivity specified
  irtkNeighbourhoodOffsets(irtkBaseImage*, irtkConnectivityType);

  /// Initializer with image and connectivity specified
  void Initialize(irtkBaseImage*, irtkConnectivityType);

  /// Initializer with slice dimensions and connectivity specified
  void Initialize(int, int, irtkConnectivityType);

  /// Default destructor
  virtual ~irtkNeighbourhoodOffsets(void);

  SetMacro(Connectivity, irtkConnectivityType);

  GetMacro(Connectivity, irtkConnectivityType);

  GetMacro(Size, int);

  //
  // Operators
  //

  int  operator()(int) const;

};

inline int irtkNeighbourhoodOffsets::operator()(int pos) const
{
#ifdef NO_BOUNDS
  return this->_Offsets[pos];
#else
  if (pos >= 0 && pos < this->_Size){
    return this->_Offsets[pos];
  } else {
    cout << "irtkNeighbourhoodOffsets::operator(): parameter out of range\n";
    return 0;
  }

#endif
}

#endif /* IRTKNEIGHBOURHOODOFFSETS_H_ */
