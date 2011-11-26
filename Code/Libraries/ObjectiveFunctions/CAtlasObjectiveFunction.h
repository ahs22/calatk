/**
*
*  Copyright 2011 by the CALATK development team
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

#ifndef C_ATLAS_OBJECTIVE_FUNCTION_H
#define C_ATLAS_OBJECTIVE_FUNCTION_H

#include "CObjectiveFunction.h"
#include "JSONParameterUtils.h"
#include "CALATKCommon.h"

namespace CALATK
{

template <class T, unsigned int VImageDimension, class TState>
class CAtlasObjectiveFunction : public CObjectiveFunction< T, VImageDimension, TState >
{
public:

  /** Some useful typedefs */

  typedef CObjectiveFunction< T, VImageDimension, TState > ObjectiveFunctionType;

  CAtlasObjectiveFunction();
  virtual ~CAtlasObjectiveFunction();

  T GetCurrentEnergy();
  void ComputeGradient();

  /**
   * @brief Sets one of the objective functions for the registration between an image and the atlas
   *
   * @param pObj - pointer to the objective function
   * @param dWeight - weight in the atlas building
   * @param uiId - id of the image
   */
  void SetObjectiveFunctionPointerAndWeight( const ObjectiveFunctionType* pObj, T dWeight, unsigned int uiId );

protected:

  void DeleteAuxiliaryStructures();
  void CreateAuxiliaryStructures();

private:
  // intentionally not implemented
  CAtlasObjectiveFunction( const CAtlasObjectiveFunction & );
  CAtlasObjectiveFunction& operator=( const CAtlasObjectiveFunction & );

  std::vector< T > vecWeights; ///< Contains the weights for each subject to atlas registration

};

#include "CAtlasObjectiveFunction.txx"

} // end namespace

#endif // C_ATLAS_OBJECTIVE_FUNCTION_H