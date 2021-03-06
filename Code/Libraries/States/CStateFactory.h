/*
*
*  Copyright 2011, 2012 by the CALATK development team
*
*   Licensed under the Apache License, Version 2.0 (the "License");
*   you may not use this file except in compliance with the License.
*   You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
*   Unless required by applicable law or agreed to in writing, software
*   distributed under the License is distributed on an "AS IS" BASIS,
*   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*   See the License for the specific language governing permissions and
*   limitations under the License.
*
*
*/

#ifndef C_STATE_FACTORY_H
#define C_STATE_FACTORY_H

#include "CState.h"
#include "CProcessBase.h"
#include "CStateInitialMomentum.h"
#include "CStateInitialImageMomentum.h"
#include "CStateScalar.h"
#include "CStateSpatioTemporalVelocityField.h"
#include "CStateMultipleStates.h"
#include "CStateImageMultipleStates.h"

namespace CALATK
{

/**
 * \brief Factory class to dynamically create different CState's.
 */
template < class TFloat, unsigned int VImageDimension=3 >
class CStateFactory : public CProcessBase
{
public:
  /** Standard class typedefs. */
  typedef CStateFactory                   Self;
  typedef CProcessBase                    Superclass;
  typedef itk::SmartPointer< Self >       Pointer;
  typedef itk::SmartPointer< const Self > ConstPointer;

  typedef TFloat                          FloatType;
  static const unsigned int ImageDimension = VImageDimension;

  // all the state typedefs
  typedef CState< FloatType >             StateType;

  typedef CStateScalar< FloatType >                                            StateScalarType;
  typedef CStateInitialMomentum< FloatType, VImageDimension >                  StateInitialMomentumType;
  typedef CStateInitialImageMomentum< FloatType, VImageDimension >             StateInitialImageMomentumType;
  typedef CStateSpatioTemporalVelocityField< FloatType, VImageDimension >      StateSpatioTemporalVelocityFieldType;
  typedef CStateMultipleStates< StateInitialMomentumType >                     StateMultipleStatesWithInitialMomentumType;
  typedef CStateMultipleStates< StateSpatioTemporalVelocityFieldType >         StateMultipleStatesWithSpatioTemporalVelocityFieldType;
  typedef CStateImageMultipleStates< StateInitialMomentumType >                StateImageMultipleStatesWithInitialMomentumType;
  typedef CStateImageMultipleStates< StateSpatioTemporalVelocityFieldType >    StateImageMultipleStatesWithSpatioTemporalVelocityFieldType;

  enum OptimizationStateType
    {
    StateScalar,
    StateInitialMomentum,
    StateInitialImageMomentum,
    StateSpatioTemporalVelocityField,
    StateMultipleStatesWithInitialMomentum,
    StateMultipleStatesWithSpatioTemporalVelocityField,
    StateImageMultipleStatesWithInitialMomentum,
    StateImageMultipleStatesWithSpatioTemporalVelocityField
    };

  CStateFactory();
  ~CStateFactory();

  static StateType* CreateNewState( const std::string & stateName );
  static StateType* CreateNewState( OptimizationStateType state );

protected:
  static OptimizationStateType GetStateTypeFromString( const std::string & stateName );
};

} // end namespace CALATK

#endif // C_STATE_FACTORY_H
