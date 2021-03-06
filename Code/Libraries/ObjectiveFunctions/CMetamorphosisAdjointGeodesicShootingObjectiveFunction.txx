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

#ifndef C_METAMORPHOSIS_ADJOINT_GEODESIC_SHOOTING_OBJECTIVE_FUNCTION_TXX
#define C_METAMORPHOSIS_ADJOINT_GEODESIC_SHOOTING_OBJECTIVE_FUNCTION_TXX

#define EXTREME_DEBUGGING
#undef EXTREME_DEBUGGING

template < class TState >
CMetamorphosisAdjointGeodesicShootingObjectiveFunction< TState >::CMetamorphosisAdjointGeodesicShootingObjectiveFunction()
  : DefaultRho( 1 ),
    m_ExternallySetRho( false ),
    m_AugmentedLagrangianMu( 0.1 ),
    m_ptrImageLagrangianMultiplier( NULL )
{
  m_Rho = DefaultRho;

  // storage for the map
  m_ptrMapIn = NULL;
  m_ptrMapOut = NULL;
  m_ptrMapTmp = NULL;

  m_ptrMapIdentity = NULL;
  m_ptrMapIncremental = NULL;

  m_ptrCurrentAdjointIDifference = NULL;

  m_ptrDeterminantOfJacobian = NULL;

  m_ptrCurrentLambdaI = NULL;
  m_ptrCurrentLambdaP = NULL;
  m_ptrCurrentLambdaV = NULL;

  // temporary storage
  m_ptrTmpField = NULL;
  m_ptrTmpFieldConv = NULL;
  m_ptrTmpScalarImage = NULL;
  m_ptrTmpImage = NULL;

  m_ptrDI = NULL;
  m_ptrDP = NULL;

  // tstLamI, tstLamP: just for testing, if EXTREME_DEBUGGING is defined it will store the full timecoarse of lamI and lamP
  // this is very memory intensive for 3D, hence disabled by default.

  // convenience pointer to the initial image (from the image manager)
  ptrI0 = NULL;
}

template< class TState >
void CMetamorphosisAdjointGeodesicShootingObjectiveFunction< TState >::DeleteData()
{
  this->m_ptrKernel->DeallocateMemory();

  ptrI0 = NULL;

  m_vecMeasurementTimepoints.clear();
  m_vecTimeDiscretization.clear();
  m_vecTimeIncrements.clear();

  this->m_ptrState = NULL;
  this->m_ptrGradient = NULL;
}

template< class TState >
CMetamorphosisAdjointGeodesicShootingObjectiveFunction< TState >::~CMetamorphosisAdjointGeodesicShootingObjectiveFunction()
{
  DeleteData();
}

template < class TState >
void CMetamorphosisAdjointGeodesicShootingObjectiveFunction< TState >::SetAutoConfiguration( CJSONConfiguration * combined, CJSONConfiguration * cleaned )
{
  Superclass::SetAutoConfiguration( combined, cleaned );
  Json::Value& currentConfigurationIn = this->m_CombinedJSONConfig->GetFromKey( "AdjointMetamorphosis", Json::nullValue );
  Json::Value& currentConfigurationOut = this->m_CleanedJSONConfig->GetFromKey( "AdjointMetamorphosis", Json::nullValue, CONF_NORMAL );

  SetJSONHelpForRootKey( GrowthModel, "settings for the adjoint metamorphosis model", CONF_NORMAL );

  SetJSONFromKeyDouble( currentConfigurationIn, currentConfigurationOut, Rho, CONF_NORMAL );

  SetJSONHelpForKey( currentConfigurationIn, currentConfigurationOut, Rho,
                     "rho is the weight of the appearance change penalty rho|q|^2", CONF_NORMAL );
}

template < class TState >
void CMetamorphosisAdjointGeodesicShootingObjectiveFunction< TState >::CreateNewStateStructures()
{
  assert( this->m_ptrState.GetPointer() == NULL );
  assert( m_vecTimeDiscretization.size() > 1 );

  // get the subject ids
  std::vector< int > vecSubjectIndices;
  this->m_ptrImageManager->GetAvailableSubjectIndices( vecSubjectIndices );

  assert( vecSubjectIndices.size()>0 );

  // obtain image from which to graft the image information for the data structures

  // get information from the first image to figure out the dimensions and determine the source and target image
  this->m_ptrState = new TState( this->m_ptrImageManager->GetGraftImagePointer( this->GetActiveSubjectId() ) );
  this->m_ptrState->GetPointerToInitialMomentum()->SetToConstant( 0 );
}

template< class TState >
void CMetamorphosisAdjointGeodesicShootingObjectiveFunction< TState >::ShallowCopyStateStructures( TState* ptrState )
{
  assert ( this->m_ptrState.GetPointer() == NULL );
  this->m_ptrState = ptrState;
}

template < class TState >
void CMetamorphosisAdjointGeodesicShootingObjectiveFunction< TState>::CreateGradientAndAuxiliaryStructures()
{
  // get the subject ids
  std::vector< int > vecSubjectIndices;
  this->m_ptrImageManager->GetAvailableSubjectIndices( vecSubjectIndices );

  assert( vecSubjectIndices.size()>0 );

  // obtain image from which to graft the image information for the data structures
  const VectorImageType* graftImage = this->m_ptrImageManager->GetGraftImagePointer( this->GetActiveSubjectId() );

  std::vector< TimeSeriesDataPointType > timeseries;
  this->m_ptrImageManager->GetTimeSeriesWithSubjectIndex( timeseries, this->GetActiveSubjectId() );
  // get information from the first image to figure out the dimensions and determine the source and target image
  ptrI0 = timeseries[0].GetImage();

  // create the gradient
  this->m_ptrGradient = new TState( graftImage );
  if ( this->m_ptrGradient->StateContainsInitialImage() )
  {
    this->m_ptrGradient->GetPointerToInitialImage()->SetToConstant( 0 );
  }
  this->m_ptrGradient->GetPointerToInitialMomentum()->SetToConstant( 0 );

  // allocate all the auxiliary data

  // image and adjoint time-series
  m_ptrI.clear();
  m_ptrP.clear();

  // for testing

#ifdef EXTREME_DEBUGGING
  tstLamI.clear();
  tstLamP.clear();
#endif

  m_ptrCurrentLambdaI = new VectorImageType( graftImage );
  m_ptrCurrentLambdaP = new VectorImageType( graftImage );
  m_ptrCurrentLambdaV = new VectorFieldType( graftImage );

  // one more than for the velocity fields
  for ( unsigned int iI=0; iI < m_vecTimeDiscretization.size(); ++iI )
    {
    typename VectorImageType::Pointer ptrCurrentVectorImage = new VectorImageType( graftImage );
    m_ptrI.push_back( ptrCurrentVectorImage );

    // bookkeeping to simplify metric computations
    m_vecTimeDiscretization[ iI ].vecEstimatedImages.push_back( ptrCurrentVectorImage );

    ptrCurrentVectorImage = new VectorImageType( graftImage );
    m_ptrP.push_back( ptrCurrentVectorImage );

#ifdef EXTREME_DEBUGGING
    // for testing
    ptrCurrentVectorImage = new VectorImageType( graftImage );
    tstLamI.push_back( ptrCurrentVectorImage );

    ptrCurrentVectorImage = new VectorImageType( graftImage );
    tstLamP.push_back( ptrCurrentVectorImage );
#endif

    }

  // storage for the maps

  m_ptrMapIn = new VectorFieldType( graftImage );
  m_ptrMapOut = new VectorFieldType( graftImage );
  m_ptrMapTmp = new VectorFieldType( graftImage );

  m_ptrMapIncremental = new VectorFieldType( graftImage );
  m_ptrMapIdentity = new VectorFieldType( graftImage );

  // temporary storage
  m_ptrTmpField = new VectorFieldType( graftImage );
  m_ptrTmpFieldConv = new VectorFieldType( graftImage );

  m_ptrTmpScalarImage = new VectorImageType( graftImage, 0.0, 1 );
  m_ptrTmpImage = new VectorImageType( graftImage );

  m_ptrDI = new VectorImageType( graftImage );
  m_ptrDP = new VectorImageType( graftImage );

  // storage for the adjoint difference

  m_ptrCurrentAdjointIDifference = new VectorImageType( graftImage );

  // storage for the determinant of Jacobian
  m_ptrDeterminantOfJacobian  = new VectorImageType( graftImage, 0.0, 1 );

  // storage for the negated velocity field
  m_ptrVelocityField.clear();
  for (unsigned int iI=0; iI < m_vecTimeDiscretization.size(); iI++)
  {
      typename VectorFieldType::Pointer ptrCurrentVectorField = new VectorFieldType( graftImage );
      ptrCurrentVectorField->SetToConstant( 0 );
      m_ptrVelocityField.push_back( ptrCurrentVectorField );

  }

  // augmented Lagrangian
  m_ptrImageLagrangianMultiplier = new VectorImageType( graftImage );
}

template < class TState >
void CMetamorphosisAdjointGeodesicShootingObjectiveFunction< TState>::InitializeDataStructuresFromState( TState* ptrState )
{
  DeleteData();

  CreateTimeDiscretization();

  ShallowCopyStateStructures( ptrState );

  CreateGradientAndAuxiliaryStructures();
}

template < class TState >
void CMetamorphosisAdjointGeodesicShootingObjectiveFunction< TState >::InitializeDataStructures()
{
  DeleteData();

  assert ( this->m_ptrGradient.GetPointer() == NULL );

  CreateTimeDiscretization();

  // allocate state structures
  CreateNewStateStructures();

  // gradient and everything else
  CreateGradientAndAuxiliaryStructures();
}

template < class TState >
void CMetamorphosisAdjointGeodesicShootingObjectiveFunction< TState >::InitializeState()
{
  InitializeDataStructures();
}

template < class TState >
void CMetamorphosisAdjointGeodesicShootingObjectiveFunction< TState >::InitializeState(TState* ptrState)
{
  InitializeDataStructuresFromState( ptrState );
}

template < class TState >
void CMetamorphosisAdjointGeodesicShootingObjectiveFunction< TState >::GetMomentum( VectorImageType* ptrMomentum, FloatType dTime )
{
  GetMap( m_ptrMapTmp, dTime );
  // now compute the momentum by interpolation
  VectorImageType* ptrInitialMomentum = this->m_ptrState->GetPointerToInitialMomentum();
  typedef LDDMMUtils< FloatType, TState::ImageDimension > LDDMMUtilsType;
  LDDMMUtilsType::applyMap( m_ptrMapTmp, ptrInitialMomentum, ptrMomentum );
  LDDMMUtilsType::computeDeterminantOfJacobian( m_ptrMapTmp, m_ptrDeterminantOfJacobian );
  ptrMomentum->MultiplyElementwise( m_ptrDeterminantOfJacobian );

}

template < class TState >
void CMetamorphosisAdjointGeodesicShootingObjectiveFunction< TState >::GetSourceImage( VectorImageType* ptrIm )
{
  throw std::runtime_error( "Not yet implemented." );
}

template < class TState >
void CMetamorphosisAdjointGeodesicShootingObjectiveFunction< TState >::GetSourceImage( VectorImageType* ptrIm, FloatType dTime )
{
  // This is more complicated than for the standard models, because we have an appearance change here
  // required an integration forward in time

  std::cout << "WARNING: GetImage for t = " << dTime << "will return the closest discretization time-point and not the value at the exact time. FIX THIS." << std::endl;

  ComputeImageMomentumForward();

  // now extract the appropriate time-point

  if ( dTime < m_vecTimeDiscretization[0].dTime || dTime > m_vecTimeDiscretization.back().dTime )
    {
    throw std::runtime_error("Requested map outside of valid time range.");
    return;
    }

  // get the desired discretization timepoint (closest to the requested one)

  unsigned int iI = 0;
  for ( iI=0; iI < m_vecTimeDiscretization.size()-1; ++iI )
    {
      if ( m_vecTimeDiscretization[ iI ].dTime >= dTime )
        {
          break;
        }
    }

  unsigned int desiredI = iI;

  if ( iI>0 )
    {
      if ( m_vecTimeDiscretization[ iI ].dTime - dTime < dTime-m_vecTimeDiscretization[ iI-1 ].dTime )
        {
          desiredI = iI;
        }
      else
        {
          desiredI = iI-1;
        }
    }

  std::cout << "desired I = " << desiredI << std::endl;

  ptrIm->Copy( m_ptrI[ desiredI ] );
}

template < class TState >
void CMetamorphosisAdjointGeodesicShootingObjectiveFunction< TState >::GetTargetImage( VectorImageType* ptrIm )
{
  throw std::runtime_error( "Not yet implemented." );
}

template < class TState >
void CMetamorphosisAdjointGeodesicShootingObjectiveFunction< TState >::GetTargetImage( VectorImageType* ptrIm, FloatType dTime )
{
  throw std::runtime_error( "Not yet implemented." );
}

template < class TState >
void CMetamorphosisAdjointGeodesicShootingObjectiveFunction< TState >::GetMap( VectorFieldType* ptrMap, FloatType dTime )
{
  FloatType dTimeFrom = m_vecTimeDiscretization[0].dTime;
  GetMapFromTo( ptrMap, dTimeFrom, dTime );

}

template < class TState >
void CMetamorphosisAdjointGeodesicShootingObjectiveFunction< TState >::GetMapFromTo( VectorFieldType* ptrMap, FloatType dTimeFrom, FloatType dTimeTo )
{
  LDDMMUtils< FloatType, TState::ImageDimension >::GetMapFromToFromSpatioTemporalVelocityField(
        ptrMap,
        dTimeFrom,
        dTimeTo,
        m_vecTimeDiscretization,
        &m_ptrVelocityField,
        this->m_ptrEvolver );
}

template< class TState >
void CMetamorphosisAdjointGeodesicShootingObjectiveFunction< TState>::CreateTimeDiscretization()
{
  if ( this->m_ptrImageManager.GetPointer() == NULL )
  {
    throw std::runtime_error( "ERROR: No image manager specified." );
    return;
  }

  std::vector< STimePoint > vecTimePointData;

  typedef LDDMMUtils< FloatType, TState::ImageDimension > LDDMMUtilsType;
  LDDMMUtilsType::DetermineTimeSeriesTimePointData( this->m_ptrImageManager, this->GetActiveSubjectId(), vecTimePointData );
  LDDMMUtilsType::CreateTimeDiscretization( vecTimePointData, m_vecTimeDiscretization, m_vecTimeIncrements, this->m_NumberOfDiscretizationVolumesPerUnitTime );

  // now add the weights, default weights are all constants here
  // TODO: make this more flexible to support local geodesic regression

  typename std::vector< STimePoint >::iterator iter;
  for ( iter = m_vecTimeDiscretization.begin(); iter != m_vecTimeDiscretization.end(); ++iter )
  {
    iter->vecWeights.clear();
    for ( unsigned int iI=0; iI<iter->vecMeasurementImages.size(); ++iI )
    {
      // TODO: make this at least a constant variable
      iter->vecWeights.push_back( 1.0/this->m_SigmaSqr );
    }
  }
  // the time discretization vector has all the N timepoint. There will be N-1 vector fields in between
}

template < class TState >
void CMetamorphosisAdjointGeodesicShootingObjectiveFunction< TState>::ComputeVelocityAdjoint( const VectorImageType * ptrI, const VectorImageType * ptrP, const VectorImageType * ptrLambdaI, const VectorImageType *  ptrLambdaP, VectorFieldType * ptrLambdaVOut)
{
/**
  * Compute the adjoint to the velocity
  * Computes
  \f[
    \lambda^v = p\nabla\lambda^p-\lambda^I\nabla I
  \f]
  */

  ptrLambdaVOut->SetToConstant(0);

  unsigned int dim = ptrI->GetDimension();

  for (unsigned int iD = 0; iD<dim; iD++)
  {
    // -\lambda^I \nabla I
    VectorFieldUtilsType::computeCentralGradient( ptrI, iD, m_ptrTmpField );
    VectorImageUtilsType::multiplyVectorByImageDimensionInPlace( ptrLambdaI, iD, m_ptrTmpField );

    ptrLambdaVOut->SubtractCellwise( m_ptrTmpField );

    // p\nabla \lambda^p

    VectorFieldUtilsType::computeCentralGradient( ptrLambdaP, iD, m_ptrTmpField );
    VectorImageUtilsType::multiplyVectorByImageDimensionInPlace( ptrP, iD, m_ptrTmpField );

    ptrLambdaVOut->AddCellwise( m_ptrTmpField );
  }
}

template < class TState >
void CMetamorphosisAdjointGeodesicShootingObjectiveFunction< TState >::ComputeImageMomentumForward()
{
  /**
    * Solves the metamorphosis version of the EPDiff equation forward in time using a map-based approach
    *
    * Note that this is more complicated than for the standard EPDiff equation because we need to deal with a source term for the image.
    * Following the method to solve for the adjoint.
    *
    * \f$ I_t + \nabla I^T v = p/rho, \f$
    * \f$ p_t + div( p v ) = 0, \f$
    * \f$ v = -K*(p\nabla I) \f$
    *
    */

  VectorImageType* ptrInitialImage = NULL;
  if ( this->m_ptrState->StateContainsInitialImage() )
  {
    ptrInitialImage = this->m_ptrState->GetPointerToInitialImage();
  }
  else
  {
    ptrInitialImage = ptrI0;
  }
  VectorImageType* ptrInitialMomentum = this->m_ptrState->GetPointerToInitialMomentum();

  typedef LDDMMUtils< FloatType, TState::ImageDimension > LDDMMUtilsType;
  LDDMMUtilsType::identityMap( m_ptrMapIdentity );

  m_ptrI[ 0 ]->Copy( ptrInitialImage );
  m_ptrP[ 0 ]->Copy( ptrInitialMomentum );

  for ( unsigned int iI = 0; iI < m_vecTimeDiscretization.size()-1; iI++ )
  {
    // compute the current velocity field
    this->ComputeVelocity( m_ptrI[ iI ], m_ptrP[iI], m_ptrVelocityField[iI], m_ptrTmpField );

    // compute the incremental map update
    this->m_ptrEvolver->SolveForward( m_ptrVelocityField[iI], m_ptrMapIdentity, m_ptrMapIncremental, m_ptrMapTmp, this->m_vecTimeIncrements[ iI ] );

    // compute the results for p and I at time-point iI+1

    // for I
    // create the current updated I (i.e., include the source term) and then warp it
    m_ptrTmpImage->Copy( m_ptrP[ iI ] );
    m_ptrTmpImage->MultiplyByConstant( this->m_vecTimeIncrements[ iI ]/m_Rho );
    m_ptrTmpImage->AddCellwise( m_ptrI[ iI ] );

    LDDMMUtilsType::applyMap( m_ptrMapIncremental, m_ptrTmpImage, m_ptrI[ iI +1 ]);

    // now create the updated p, this is just warping p forward in time
    LDDMMUtilsType::applyMap( m_ptrMapIncremental, m_ptrP[ iI ], m_ptrP[ iI+1 ]);
    // adjust with respect to the warp (i.e., multiply with the determinant of the Jacobian)
    LDDMMUtilsType::computeDeterminantOfJacobian( m_ptrMapIncremental, m_ptrDeterminantOfJacobian );
    m_ptrP[iI+1]->MultiplyElementwise( m_ptrDeterminantOfJacobian );
  }
}

template < class TState >
void CMetamorphosisAdjointGeodesicShootingObjectiveFunction< TState >::ComputeAdjointsBackward()
{
 /**
  * Computes the adjoint equations backward for metamorphosis.
  * Equations are similar as for the adjoint image-to-image registration case, but include the contributions of the augmented Lagrangian scheme.
  *
  * \f$ -\lambda_t^I -div(v\lambda^I)- div(pK*\lambda^v) = 0 \f$
  * \f$ -\lambda_t^p -v^T\nabla\lambda^p + \nabla I^T K*\lambda^v = 1/rho\lambda^I \f$
  * \f$ \lambda^v = p\nabla \lambda^p - \lambda^I\nabla I \f$
  *
  * with final conditions
  *
  * \f$ \lambda^p(1) = 0, \lambda^I(1) = r-\mu(I(1)-I_1) \f$
  */

  VectorImageType* ptrInitialImage = NULL;
  if ( this->m_ptrState->StateContainsInitialImage() )
  {
    ptrInitialImage = this->m_ptrState->GetPointerToInitialImage();
  }
  else
  {
    ptrInitialImage = ptrI0;
  }

  // map all the temporary variables to variables with meaningful names for this method
  VectorFieldType* ptrCurrentVelocityField = m_ptrTmpField;
  VectorFieldType* ptrCurrentKLambdaV = m_ptrTmpFieldConv;

  unsigned int uiNrOfTimePoints = m_vecTimeDiscretization.size();
  unsigned int uiNrOfMeasuredImagesAtFinalTimePoint;
  unsigned int dim = ptrInitialImage->GetDimension();

  uiNrOfMeasuredImagesAtFinalTimePoint = m_vecTimeDiscretization[ uiNrOfTimePoints - 1].vecMeasurementImages.size();

  // TODO: write general checks for methods that make sure that they are only run if the underlying assumptions hold
  // TODO: also check that there are really only two images (one source and one target; otherwise this method does not work)
  assert( uiNrOfMeasuredImagesAtFinalTimePoint == 1 );

  // compute the final conditions
  // \f$ \lambda^I(t_end) = r-\mu(I(1)-I_1)\f$
  // \f$ \lambda^p(t_end) = 0

  // create the final condition for the image adjoint \lambda^I

  m_ptrCurrentLambdaI->SetToConstant( 0.0 );
  m_ptrCurrentLambdaP->SetToConstant( 0.0 );

  // I_1
  m_ptrCurrentLambdaI->Copy( m_vecTimeDiscretization[ uiNrOfTimePoints - 1].vecMeasurementImages[ 0 ] );
  // -I(1)
  m_ptrCurrentLambdaI->AddCellwiseMultiply( m_vecTimeDiscretization[ uiNrOfTimePoints-1].vecEstimatedImages[ 0 ], -1.0 );
  // \mu(I_1-I(1))
  m_ptrCurrentLambdaI->MultiplyByConstant( m_AugmentedLagrangianMu );
  // add r
  m_ptrCurrentLambdaI->AddCellwise( m_ptrImageLagrangianMultiplier );

  // initialize the initial condition for the incremental map used to flow backwards. Will not be changed during
  // the iterations, because the numerical solution is always started from the identity
  // m_ptrMapIncremental maps bewtween different time discretization points; used for the source terms
  LDDMMUtils< FloatType, TState::ImageDimension >::identityMap( m_ptrMapIdentity );

#ifdef EXTREME_DEBUGGING
  tstLamI[ m_vecTimeDiscretization.size()-1 ]->copy( m_ptrCurrentLambdaI );
  tstLamP[ m_vecTimeDiscretization.size()-1 ]->copy( m_ptrCurrentLambdaP );
#endif

  typedef LDDMMUtils< FloatType, TState::ImageDimension >      LDDMMUtilsType;

  for ( int iI = (int)m_vecTimeDiscretization.size()-1-1; iI>=0; iI--)
  {
    // use the velocity field computed in the forward direction, but negate it
    ptrCurrentVelocityField->Copy( m_ptrVelocityField[ iI ] );
    ptrCurrentVelocityField->MultiplyByConstant(-1);

    // update the incremental map
    this->m_ptrEvolver->SolveForward( ptrCurrentVelocityField, m_ptrMapIdentity, m_ptrMapIncremental, m_ptrMapTmp, this->m_vecTimeIncrements[iI] );

    // compute the convolved adjoint velocity at time point i+1
    // this introduces a slight assymmetry in the solution (because ideally we would like to compute it at i)
    // compute K*\lambda_v; first lambda_v

    this->ComputeVelocityAdjoint( m_ptrI[ iI+1 ], m_ptrP[ iI+1 ], m_ptrCurrentLambdaI, m_ptrCurrentLambdaP, ptrCurrentKLambdaV );

    // compute K*\lambda_v
    this->m_ptrKernel->ConvolveWithKernel( ptrCurrentKLambdaV );

    // compute di = div(p K*lambda_v) and dp = -\nabla I^T ( K* lambda_v ) + \lambda^I

    m_ptrDI->SetToConstant( 0 );
    m_ptrDP->SetToConstant( 0 );

    for ( unsigned int iD=0; iD<dim; iD++ )
    {
      // 1) compute dp = -\nabla I^T ( K* lambda_v ) + \lambda^I
      // nabla I
      VectorFieldUtilsType::computeCentralGradient( m_ptrI[ iI+1 ], iD, m_ptrTmpField );

      // compute the element-wise inner product
      VectorImageUtilsType::multiplyVectorByVectorInnerProductElementwise( m_ptrTmpField, ptrCurrentKLambdaV, m_ptrTmpScalarImage );

      // account for the time-increment
      m_ptrTmpScalarImage->MultiplyByConstant( -1.0 );
      // store it in the i-th dimension of dp
      VectorImageUtilsType::addScalarImageToVectorImageAtDimensionInPlace( m_ptrTmpScalarImage, m_ptrDP, iD );

      // 2) Compute di = div( p K*lambda_v )
      m_ptrTmpField->Copy( ptrCurrentKLambdaV );
      // multiply it by the d-th dimension of p
      VectorImageUtilsType::multiplyVectorByImageDimensionInPlace( m_ptrP[ iI+1 ], iD, m_ptrTmpField );

      // compute the divergence
      VectorFieldUtilsType::computeDivergence( m_ptrTmpField, m_ptrTmpScalarImage );

      // store it in the i-th dimension of di
      VectorImageUtilsType::addScalarImageToVectorImageAtDimensionInPlace( m_ptrTmpScalarImage, m_ptrDI, iD );
    }

    // add \lambda^I to DP
    m_ptrDP->AddCellwiseMultiply( m_ptrCurrentLambdaI, 1.0/m_Rho );

    // account for the time-difference
    m_ptrDI->MultiplyByConstant( m_vecTimeIncrements[ iI ] );
    m_ptrDP->MultiplyByConstant( m_vecTimeIncrements[ iI ] );

    // now add up all the contributions that happened during this time step

    m_ptrCurrentLambdaI->AddCellwise( m_ptrDI );
    m_ptrCurrentLambdaP->AddCellwise( m_ptrDP );

    LDDMMUtilsType::computeDeterminantOfJacobian( m_ptrMapIncremental, m_ptrDeterminantOfJacobian );

    // 1) account for the deformation in this time increment; just push via map by one timestep
    LDDMMUtilsType::applyMap( m_ptrMapIncremental, m_ptrCurrentLambdaI, m_ptrTmpImage );
    m_ptrCurrentLambdaI->Copy( m_ptrTmpImage );
    m_ptrCurrentLambdaI->MultiplyElementwise( m_ptrDeterminantOfJacobian );

    LDDMMUtilsType::applyMap( m_ptrMapIncremental, m_ptrCurrentLambdaP,  m_ptrTmpImage );
    m_ptrCurrentLambdaP->Copy( m_ptrTmpImage );

    // no jumps necessary. we assume initial image is exact and there are no measurements in between

#ifdef EXTREME_DEBUGGING
    tstLamI[ iI ]->copy( m_ptrCurrentLambdaI );
    tstLamP[ iI ]->copy( m_ptrCurrentLambdaP );
#endif

  }
}

template < class TState >
void CMetamorphosisAdjointGeodesicShootingObjectiveFunction< TState>::ComputeGradient()
{
  // TODO: Need a state that only holds the momentum: change of image is not meaningful here

  ComputeImageMomentumForward();
  ComputeAdjointsBackward();

  /**
    * The gradient is computed as follows. Image gradient is not meaningful for metamorphosis
    *
    \f[
      \nabla_{I_0(t_0)}E = 0
    \f]
    *
    \f[
      \nabla_{p(t_0)}E = -\lambda^p(t_0) + \nabla I(t_0)^T K* ( p(t_0)\nabla I(t_0) ) + 1/rho*p(t_0)
    \f]
    */

  VectorImageType* ptrInitialImage = NULL;
  if ( this->m_ptrState->StateContainsInitialImage() )
  {
    ptrInitialImage = this->m_ptrState->GetPointerToInitialImage();
  }
  else
  {
    ptrInitialImage = ptrI0;
  }
  VectorImageType* ptrInitialMomentum = this->m_ptrState->GetPointerToInitialMomentum();

  VectorImageType* ptrI0Gradient = NULL;
  if ( this->m_ptrGradient->StateContainsInitialImage() )
  {
    ptrI0Gradient = this->m_ptrGradient->GetPointerToInitialImage();
    ptrI0Gradient->SetToConstant( 0.0 );
    // TODO: support actual gradient with respect to I0
  }
  VectorImageType* ptrP0Gradient = this->m_ptrGradient->GetPointerToInitialMomentum();

  ptrP0Gradient->Copy( m_ptrCurrentLambdaP );
  ptrP0Gradient->MultiplyByConstant(-1);

  ptrP0Gradient->AddCellwiseMultiply( ptrInitialMomentum, 1.0/m_Rho );

  unsigned int dim = ptrInitialImage->GetDimension();

  for ( unsigned int iD=0; iD<dim; iD++ )
  {
    // compute the gradient with respect to the d-th dimension of p; -\lambda^p(t_0) + \nabla I(t_0)^T K* ( p(t_0)\nabla I(t_0) )

    // nabla I
    VectorFieldUtilsType::computeCentralGradient( ptrInitialImage, iD, m_ptrTmpField );
    m_ptrTmpFieldConv->Copy( m_ptrTmpField );
    // multiply with initial momentum corresponding to this dimension of the image; p_i \nabla I
    VectorImageUtilsType::multiplyVectorByImageDimensionInPlace( ptrInitialMomentum, iD, m_ptrTmpFieldConv );

    // convolve with kernel; CAN BE OPTIMIZED HERE, JUST CONVOLVE AFTER SUMMING
    this->m_ptrKernel->ConvolveWithKernel( m_ptrTmpFieldConv );

    // compute the element-wise inner product; \nabla I^ K*(p_i\nabla I)
    VectorImageUtilsType::multiplyVectorByVectorInnerProductElementwise( m_ptrTmpField, m_ptrTmpFieldConv, m_ptrTmpScalarImage );

    // now add it to the p0 gradient
    VectorImageUtilsType::addScalarImageToVectorImageAtDimensionInPlace( m_ptrTmpScalarImage, ptrP0Gradient, iD );
  }

  ptrP0Gradient->MultiplyByConstant( this->m_EnergyWeight );

}

template < class TState >
void CMetamorphosisAdjointGeodesicShootingObjectiveFunction< TState >::ComputeInitialUnsmoothedVelocityGradient( VectorFieldType* ptrInitialUnsmoothedVelocityGradient, unsigned int uiKernelNumber )
{

  std::cout << "WARNING: initial unsmooth gradient computation is NOT adapted for metamorphosis. Results may be inaccurate. FIXME." << std::endl;

  // compute the unsmoothed velocity gradient; to be used to estimate weights for the multi-Gaussian kernels.
  // this is an approximation based on adding the augmented Lagrangian constraint to the original model (rather than the 2nd order for shooting)
  // v, r and p(0) is assumed zero here and the unsmoothed gradient is then, because I(t) = I(0) and p(t) = p(1)

  // \f$ \nabla E = \sum_i \mu(I_0-I_1)\nabla I_0  \f$
  // sum is over dimensions

  VectorImageType* ptrCurrentI0 = NULL;
  if ( this->m_ptrState->StateContainsInitialImage() )
  {
    ptrCurrentI0 = this->m_ptrState->GetPointerToInitialImage();
  }
  else
  {
    ptrCurrentI0 = ptrI0;
  }
  unsigned int dim = ptrCurrentI0->GetDimension();

  unsigned int uiNrOfDiscretizationPoints = this->m_vecTimeDiscretization.size();
  VectorImageType* ptrI1 =  this->m_vecTimeDiscretization[ uiNrOfDiscretizationPoints-1 ].vecMeasurementImages[ 0 ];

  // initialize to 0
  VectorFieldType* ptrCurrentGradient = ptrInitialUnsmoothedVelocityGradient;
  ptrCurrentGradient->SetToConstant( 0 );

  // \mu(I_0-I_1)
  m_ptrTmpImage->Copy( ptrCurrentI0 );
  m_ptrTmpImage->AddCellwiseMultiply( ptrI1, -1.0 );
  m_ptrTmpImage->MultiplyByConstant( m_AugmentedLagrangianMu );

  for ( unsigned int iD = 0; iD<dim; ++iD )
    {
    VectorFieldUtilsType::computeCentralGradient( ptrCurrentI0, iD, m_ptrTmpField );
    VectorImageUtilsType::multiplyVectorByImageDimensionInPlace( m_ptrTmpImage, iD, m_ptrTmpField );
    ptrCurrentGradient->AddCellwise( m_ptrTmpField );
    }

  ptrCurrentGradient->MultiplyByConstant( this->m_EnergyWeight );

}

template < class TState >
typename CMetamorphosisAdjointGeodesicShootingObjectiveFunction< TState >::CEnergyValues
CMetamorphosisAdjointGeodesicShootingObjectiveFunction< TState >::GetCurrentEnergy()
{
  /**
    * Computes the energy for the shooting method.
    \f[
      E = 0.5 \langle p(t_0)\nabla I(t_0),K*(p(t_0)\nabla I(t_0)\rangle + 0.5 rho \langle p(t_0), p(t_0) \rangle -\langle r, I(1)-I_1\rangle + \frac{\mu}{2}\|I_1-I(1)\|^2
    \f]
  */
  FloatType dEnergy = 0;

  // computing \f$ 0.5\langle p(t_0) \nabla I(t_0) +  K*( p(t_0)\nabla I(t_0) ) \rangle \f$
  // this is done dimension for dimension (i.e., if we have a multidimensional image, we have as many of these terms as we have dimensions)

  VectorImageType* ptrInitialImage = NULL;
  if ( this->m_ptrState->StateContainsInitialImage() )
  {
    ptrInitialImage = this->m_ptrState->GetPointerToInitialImage();
  }
  else
  {
    ptrInitialImage = ptrI0;
  }
  VectorImageType* ptrInitialMomentum = this->m_ptrState->GetPointerToInitialMomentum();

  unsigned int dim = ptrInitialImage->GetDimension();

  for ( unsigned int iD=0; iD<dim; iD++ )
  {
    // nabla I
    VectorFieldUtilsType::computeCentralGradient( ptrInitialImage, iD, m_ptrTmpField );
    // multiply with initial momentum corresponding to this dimension of the image; p_i\nabla I
    VectorImageUtilsType::multiplyVectorByImageDimensionInPlace( ptrInitialMomentum, iD, m_ptrTmpField );

    // convolve with kernel, K*(pi\nabla I)
    m_ptrTmpFieldConv->Copy( m_ptrTmpField );
    this->m_ptrKernel->ConvolveWithKernel( m_ptrTmpFieldConv );

    // now we need to compute the inner product
    dEnergy += m_ptrTmpField->ComputeInnerProduct( m_ptrTmpFieldConv );
  }

  // now add the simply the squared norm of the initial momentum

  dEnergy += m_Rho*ptrInitialMomentum->ComputeSquaredNorm();

  // multiply the full energy by 0.5
  unsigned int uiNrOfDiscretizationPoints = m_vecTimeDiscretization.size();
  FloatType dTimeDuration = this->m_vecTimeDiscretization[ uiNrOfDiscretizationPoints-1 ].dTime - this->m_vecTimeDiscretization[0].dTime;

  dEnergy *= 0.5*dTimeDuration;
  dEnergy *= this->m_EnergyWeight;

  FloatType dVelocitySquareNorm = dEnergy;

  // add the contributions of the data terms (of the augmented Lagrangian)

  ComputeImageMomentumForward();

  // computing I(1)-I_1
  m_ptrTmpImage->Copy( m_vecTimeDiscretization[ uiNrOfDiscretizationPoints-1].vecEstimatedImages[ 0 ] );
  m_ptrTmpImage->AddCellwiseMultiply( m_vecTimeDiscretization[ uiNrOfDiscretizationPoints - 1].vecMeasurementImages[ 0 ], -1.0 );

  FloatType dImageNorm = m_ptrTmpImage->ComputeSquaredNorm();

  // +\mu/2\|I(1)-I_1\|^2
  FloatType dAugmentedLagrangianNorm = 0.5*m_AugmentedLagrangianMu*dImageNorm;

  // -<r,I(1)-I_1>
  dAugmentedLagrangianNorm -= m_ptrTmpImage->ComputeInnerProduct( m_ptrImageLagrangianMultiplier );

  dEnergy += dAugmentedLagrangianNorm*this->m_EnergyWeight;

  CEnergyValues energyValues;
  energyValues.dEnergy = dEnergy;
  energyValues.dRegularizationEnergy = dVelocitySquareNorm;
  energyValues.dMatchingEnergy = dImageNorm*this->m_EnergyWeight;

  return energyValues;

}

template < class TState >
void CMetamorphosisAdjointGeodesicShootingObjectiveFunction< TState >::OutputStateInformation( unsigned int uiIter, std::string outputPrefix )
{
  std::cout << "saving gradient components at iteration " << uiIter << std::endl;

  outputPrefix = outputPrefix + "Shooting-";

  ComputeImageMomentumForward();
  ComputeAdjointsBackward();

  unsigned int dim = m_ptrTmpImage->GetDimension();

  typename VectorFieldType::Pointer ptrCurrentGradient = new VectorFieldType( m_ptrTmpField );

  std::string suffix = "-iter-"  + CreateIntegerString( uiIter, 3 ) + ".nrrd";

  // write out the kernel
  VectorImageUtilsType::writeFileITK( this->m_ptrKernel->GetKernel(), outputPrefix + "Kernel" + CreateIntegerString( 0, 3 ) + suffix );

  // write out the currently stored gradient

  if ( this->m_ptrGradient->StateContainsInitialImage() )
  {
    VectorImageType* ptrI0Gradient = this->m_ptrGradient->GetPointerToInitialImage();
    VectorImageUtilsType::writeFileITK( ptrI0Gradient, outputPrefix + "I0Gradient" + CreateIntegerString( 0, 3 ) + suffix );
  }

  VectorImageType* ptrP0Gradient = this->m_ptrGradient->GetPointerToInitialMomentum();

  VectorImageUtilsType::writeFileITK( ptrP0Gradient, outputPrefix + "P0Gradient" + CreateIntegerString( 0, 3 ) + suffix );

  for ( unsigned int iI = 0; iI < this->m_vecTimeDiscretization.size()-1; ++iI )
    {
    // initialize to 0
    ptrCurrentGradient->SetToConstant( 0 );

    VectorImageUtilsType::writeFileITK( m_ptrI[iI], outputPrefix + "I" + CreateIntegerString( iI, 3 ) + suffix );
    VectorImageUtilsType::writeFileITK( m_ptrP[iI], outputPrefix + "lam" + CreateIntegerString( iI, 3 ) + suffix );

    for ( unsigned int iD = 0; iD<dim; ++iD )
      {
      VectorFieldUtilsType::computeCentralGradient( m_ptrI[ iI ], iD, m_ptrTmpField );
      VectorImageUtilsType::writeFileITK( m_ptrTmpField, outputPrefix + "gradI" + CreateIntegerString( iI, 3 ) + suffix );

      VectorImageUtilsType::multiplyVectorByImageDimensionInPlace( m_ptrP[ iI ], iD, m_ptrTmpField );
      VectorImageUtilsType::writeFileITK( m_ptrTmpField, outputPrefix + "lam_x_gradI" + CreateIntegerString( iI, 3 ) + suffix );

      ptrCurrentGradient->AddCellwise( m_ptrTmpField );
      }

    this->m_ptrKernel->ConvolveWithKernel( ptrCurrentGradient );

    VectorImageUtilsType::writeFileITK( ptrCurrentGradient, outputPrefix + "conv_lam_x_gradI" + CreateIntegerString( iI, 3 ) + suffix );

    // add v
    this->ComputeVelocity( m_ptrI[ iI ], m_ptrP[ iI ], m_ptrTmpField, m_ptrTmpFieldConv );

    VectorImageUtilsType::writeFileITK( m_ptrTmpField, outputPrefix + "v" + CreateIntegerString( iI, 3 ) + suffix );

    ptrCurrentGradient->AddCellwise( m_ptrTmpField );
    VectorImageUtilsType::writeFileITK( ptrCurrentGradient, outputPrefix + "gradv" + CreateIntegerString( iI, 3 ) + suffix );

#ifdef EXTREME_DEBUGGING
    VectorImageUtilsType::writeFileITK( tstLamI[iI], outputPrefix + "lamI" + CreateIntegerString( iI, 3) + suffix );
    VectorImageUtilsType::writeFileITK( tstLamP[iI], outputPrefix + "lamP" + CreateIntegerString( iI, 3) + suffix );
#endif
    }
}

/// functionality for augmented Lagrangian
template < class TState >
void CMetamorphosisAdjointGeodesicShootingObjectiveFunction< TState >::SetSquaredPenaltyScalarWeight( FloatType dWeight )
{
  m_AugmentedLagrangianMu = dWeight;
}

template < class TState >
typename CMetamorphosisAdjointGeodesicShootingObjectiveFunction< TState >::FloatType
CMetamorphosisAdjointGeodesicShootingObjectiveFunction< TState >::GetSquaredPenaltyScalarWeight()
{
  return m_AugmentedLagrangianMu;
}

template < class TState >
typename CMetamorphosisAdjointGeodesicShootingObjectiveFunction< TState >::VectorImageType*
CMetamorphosisAdjointGeodesicShootingObjectiveFunction< TState >::GetPointerToImageLagrangianMultiplier()
{
  return m_ptrImageLagrangianMultiplier;
}

template < class TState >
const typename CMetamorphosisAdjointGeodesicShootingObjectiveFunction< TState >::VectorImageType*
CMetamorphosisAdjointGeodesicShootingObjectiveFunction< TState >::GetPointerToCurrentImageResidual()
{
  // TODO: extend to support multiple time-points (how?)
  // just look at the last time point for now here
  unsigned int uiNrOfTimePoints= m_vecTimeDiscretization.size();
  unsigned int uiNrOfMeasuredImagesAtFinalTimePoint = m_vecTimeDiscretization[ uiNrOfTimePoints - 1].vecMeasurementImages.size();

  // for now we can only deal with one image (to be fixed)
  assert( uiNrOfMeasuredImagesAtFinalTimePoint==1 );

  // computing I(1)-I_1
  m_ptrTmpImage->Copy( m_vecTimeDiscretization[ uiNrOfTimePoints - 1].vecMeasurementImages[ 0 ] );
  m_ptrTmpImage->MultiplyByConstant( -1.0 );
  m_ptrTmpImage->AddCellwise( m_vecTimeDiscretization[ uiNrOfTimePoints-1].vecEstimatedImages[ 0 ] );

  return m_ptrTmpImage;

}


#endif
