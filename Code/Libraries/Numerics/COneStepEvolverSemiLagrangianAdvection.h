#ifndef C_ONESTEPEVOLVER_SEMILAGRANGIAN_ADVECTION_H
#define C_ONESTEPEVOLVER_SEMILAGRANGIAN_ADVECTION_H

#include <stdexcept>

#include "COneStepEvolver.h"
#include "VectorFieldUtils.h"

/**
 * COneStepEvolverSemiLagrangianAdvection.h -- implementation of a one-step advection 
 * evolver using a semi-lagrangian approach
 *
 */

// FIXME: Do a proper semi-lagrangian implementation. This one is numerically not very accurate

namespace CALATK
{

template <class T, unsigned int VImageDimension=3, class TSpace = T >
class COneStepEvolverSemiLagrangianAdvection : public COneStepEvolver< T, VImageDimension, TSpace > {

public:

  typedef COneStepEvolver< T, VImageDimension, TSpace > Superclass;
  typedef typename Superclass::VectorFieldType VectorFieldType;
  typedef typename Superclass::VectorImageType VectorImageType;

  /**
   * Empty Constructor
   */
  COneStepEvolverSemiLagrangianAdvection();

  virtual void PerformStep( const VectorFieldType* v, const VectorImageType* In, VectorImageType* Inp1, T dt );

  T ComputeMaximalUpdateStep( const VectorFieldType* v ) const;

protected:

  void PerformStep2D( const VectorFieldType* v, const VectorImageType* In, VectorImageType* Inp1, T dt );
  void PerformStep3D( const VectorFieldType* v, const VectorImageType* In, VectorImageType* Inp1, T dt );

private:

};

#include "COneStepEvolverSemiLagrangianAdvection.txx"

} // end namespace

#endif
