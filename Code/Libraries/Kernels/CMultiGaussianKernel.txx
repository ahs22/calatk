/*
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

#ifndef C_MULTI_GAUSSIAN_KERNEL_TXX
#define C_MULTI_GAUSSIAN_KERNEL_TXX

template <class T, unsigned int VImageDimension >
CMultiGaussianKernel< T, VImageDimension >::CMultiGaussianKernel()
  : m_ExternallySetSigmas( false ),
    m_ExternallySetEffectiveWeights( false ),
    m_ExternallySetEstimateGradientScalingFactors( true )
{
  DefaultSigmas.resize( 5 );
  DefaultSigmas[ 0 ] = 0.25;
  DefaultSigmas[ 1 ] = 0.20;
  DefaultSigmas[ 2 ] = 0.15;
  DefaultSigmas[ 3 ] = 0.10;
  DefaultSigmas[ 4 ] = 0.05;

  DefaultEffectiveWeights.resize( 5, 1.0 );
  DefaultGradientScalingFactors.resize( 5, 1.0 );

  m_Sigmas = DefaultSigmas;
  m_EffectiveWeights = DefaultEffectiveWeights;
  m_EstimateGradientScalingFactors = DefaultEstimateGradientScalingFactors;

  m_GradientScalingFactors = DefaultGradientScalingFactors;

}

template <class T, unsigned int VImageDimension >
CMultiGaussianKernel< T, VImageDimension >::~CMultiGaussianKernel()
{
}

template<class T, unsigned int VImageDimension >
void CMultiGaussianKernel< T, VImageDimension >::SetAutoConfiguration( Json::Value& ConfValueIn, Json::Value& ConfValueOut )
{
  Superclass::SetAutoConfiguration( ConfValueIn, ConfValueOut );
  Json::Value& currentConfigurationIn = this->m_jsonConfigIn.GetFromKey( "MultiGaussianKernel", Json::nullValue );
  Json::Value& currentConfigurationOut = this->m_jsonConfigOut.GetFromKey( "MultiGaussianKernel", Json::nullValue );

  SetJSONFromKeyVector( currentConfigurationIn, currentConfigurationOut, Sigmas );
  SetJSONFromKeyVector( currentConfigurationIn, currentConfigurationOut, EffectiveWeights );
  SetJSONFromKeyBool( currentConfigurationIn, currentConfigurationOut, EstimateGradientScalingFactors );

}


template <class T, unsigned int VImageDimension >
void CMultiGaussianKernel< T, VImageDimension >::ComputeActualWeights()
{
  // check that all arrays have the correct size (or if not) make up reasonable default values
  if ( m_Sigmas.empty() )
  {
    throw std::runtime_error( "CMultiGaussianKernel: at least one Gaussian kernel needs to be defined.");
    return;
  }
  if ( m_Sigmas.size() != m_EffectiveWeights.size() )
  {
    throw std::runtime_error( "CMultiGaussianKernel: dimension of standard deviations and of the effective weights disagrees.");
    return;
  }
  else
  {
    if ( m_GradientScalingFactors.empty() )
    {
      // just initializing with one
      m_GradientScalingFactors.resize( m_EffectiveWeights.size(), 1.0 );
    }
    // now we can compute the actual weights
    m_ActualWeights.clear();
    for ( unsigned int iI=0; iI<m_EffectiveWeights.size(); ++iI )
    {
      m_ActualWeights.push_back( m_EffectiveWeights[ iI ]/m_GradientScalingFactors[ iI ] );
    }
  }

  // scale the weights so they sum up to one

  T dSum = 0;
  for ( unsigned int iI=0; iI < m_ActualWeights.size(); ++iI )
  {
    dSum += m_ActualWeights[ iI ];
  }

  if ( dSum >0 )
  {
    for ( unsigned int iI=0; iI < m_ActualWeights.size(); ++iI )
    {
      m_ActualWeights[ iI ] /= dSum;
    }
  }
  else
  {
    throw std::runtime_error( "Gaussian multi kernel weights sum up to zero. Cannot be normalized.");
  }

  std::cout << "actual weights, set : ";
  for ( unsigned int iI=0; iI<m_ActualWeights.size(); ++iI )
  {
    std::cout << m_ActualWeights[ iI ] << " ";
  }
  std::cout << std::endl;

  ConfirmKernelsNeedToBeComputed();
}

template <class T, unsigned int VImageDimension >
void CMultiGaussianKernel< T, VImageDimension >::SetSigmasAndEffectiveWeights( std::vector<T> Sigmas, std::vector<T> EffectiveWeights )
{
  m_Sigmas = Sigmas;
  m_EffectiveWeights = EffectiveWeights;
  ComputeActualWeights();

  m_ExternallySetSigmas = true;
  m_ExternallySetEffectiveWeights = true;

  ConfirmKernelsNeedToBeComputed();
}

template <class T, unsigned int VImageDimension >
void CMultiGaussianKernel< T, VImageDimension >::ComputeKernelAndInverseKernel( VectorImageType1D* pVecImageGraft )
{
  unsigned int szX = pVecImageGraft->getSizeX();

  T dx = pVecImageGraft->getSpaceX();

  T pi = (T)CALATK::PI;

  T f1Eff = 0;

  for (unsigned int x = 0; x < szX; ++x)
    {
    f1Eff = GetFFromIndex( x, szX, dx );

    T val = 0;

    for ( unsigned int iI=0; iI<m_ActualWeights.size(); ++iI )
    {
      val += m_ActualWeights[ iI ]*exp( -m_Sigmas[ iI ]*m_Sigmas[ iI ]*( 4*pi*pi*(f1Eff*f1Eff)/2 ) );
    }

    this->m_ptrL->setValue(x,0, val );

    // avoid division by zero!!
    if ( val <= std::numeric_limits<T>::epsilon() )
      {
      this->m_ptrLInv->setValue(x,0,1.0/std::numeric_limits<T>::epsilon() );
      }
    else
      {
      this->m_ptrLInv->setValue(x,0,1.0/(val) );
      }
    }

}


template <class T, unsigned int VImageDimension >
void CMultiGaussianKernel< T, VImageDimension >::ComputeKernelAndInverseKernel( VectorImageType2D* pVecImageGraft )
{
  unsigned int szX = pVecImageGraft->getSizeX();
  unsigned int szY = pVecImageGraft->getSizeY();

  T dx = pVecImageGraft->getSpaceX();
  T dy = pVecImageGraft->getSpaceY();

  T pi = (T)CALATK::PI;

  T f1Eff = 0;
  T f2Eff = 0;

  for (unsigned int y = 0; y < szY; ++y)
    {
    f2Eff = GetFFromIndex( y, szY, dy );
    for (unsigned int x = 0; x < szX; ++x)
      {
      f1Eff = GetFFromIndex( x, szX, dx );

      T val = 0;

      for ( unsigned int iI=0; iI<m_ActualWeights.size(); ++iI )
      {
        val += m_ActualWeights[ iI ]*exp( -m_Sigmas[ iI ]*m_Sigmas[ iI ]*( 4*pi*pi*(f1Eff*f1Eff + f2Eff*f2Eff )/2 ) );
      }

      this->m_ptrL->setValue(x,y,0, val );

      // avoid division by zero!!
      if ( val <= std::numeric_limits<T>::epsilon() )
        {
        this->m_ptrLInv->setValue(x,y,0,1.0/std::numeric_limits<T>::epsilon() );
        }
      else
        {
        this->m_ptrLInv->setValue(x,y,0,1.0/(val) );
        }
      }
    }

}

template <class T, unsigned int VImageDimension >
void CMultiGaussianKernel< T, VImageDimension >::ComputeKernelAndInverseKernel( VectorImageType3D* pVecImageGraft )
{

  unsigned int szX = pVecImageGraft->getSizeX();
  unsigned int szY = pVecImageGraft->getSizeY();
  unsigned int szZ = pVecImageGraft->getSizeZ();

  T dx = pVecImageGraft->getSpaceX();
  T dy = pVecImageGraft->getSpaceY();
  T dz = pVecImageGraft->getSpaceZ();

  T pi = (T)CALATK::PI;

  T f1Eff = 0;
  T f2Eff = 0;
  T f3Eff = 0;

  for (unsigned int z = 0; z < szZ; ++z)
    {
    f3Eff = GetFFromIndex( z, szZ, dz );
    for (unsigned int y = 0; y < szY; ++y)
      {
      f2Eff = GetFFromIndex( y, szY, dy );
      for (unsigned int x = 0; x < szX; ++x)
        {
        f1Eff = GetFFromIndex( x, szX, dx );

        T val = 0;

        for ( unsigned int iI=0; iI<m_ActualWeights.size(); ++iI )
        {
          val += m_ActualWeights[ iI ]*exp( -m_Sigmas[ iI ]*m_Sigmas[ iI ]*( 4*pi*pi*(f1Eff*f1Eff + f2Eff*f2Eff + f3Eff*f3Eff )/2 ) );
        }

        this->m_ptrL->setValue(x,y,z,0, val );

        // avoid division by zero!!
        if ( val <= std::numeric_limits<T>::epsilon() )
          {
          this->m_ptrLInv->setValue(x,y,z,0,1.0/std::numeric_limits<T>::epsilon() );
          }
        else
          {
          this->m_ptrLInv->setValue(x,y,z,0,1.0/(val) );
          }
        }
      }
    }
}

template<class T, unsigned int VImageDimension >
std::vector< T > CMultiGaussianKernel< T, VImageDimension >::ComputeDataDependentScalingFactors()
{
  std::vector< T > vecWeights;

  unsigned int uiNrOfSigmas = m_Sigmas.size();
  if ( uiNrOfSigmas==0 )
  {
    std::cerr << "Sigmas for multi-Gaussian kernel have not been set. Cannot compute the weights." << std::endl;
    // return an empty vector
    return vecWeights;
  }

  if ( this->ptrObjectiveFunction == NULL )
  {
    // cannot determine this in a data-driven manner; just initialize to constants
    std::cerr << "Cannot compute data-dependent scaling factors for multi-Gaussian kernels, because objective function was not set." << std::endl;
    // return an empty vector for now
    return vecWeights;
  }
  else
  {
    // initialize a vector field of appropriate size
    VectorFieldType *ptrGradient = new VectorFieldType( this->ptrObjectiveFunction->GetPointerToInitialImage() );

    this->ptrObjectiveFunction->ComputeInitialUnsmoothedVelocityGradient( ptrGradient );
    // now go through all the sigmas and determine what the weights should be
    VectorFieldType *ptrSmoothedGradient = new VectorFieldType( ptrGradient );

    for ( unsigned int iI=0; iI < uiNrOfSigmas; ++iI )
    {
      std::cout << "Computing multi-Gaussian kernel weight for sigma = " << m_Sigmas[ iI ] << " ... ";

      // instantiate a Gaussian kernel with appropriate sigma
      CGaussianKernel< T, VImageDimension > gaussianKernel;
      gaussianKernel.SetSigma( m_Sigmas[ iI ] );

      ptrSmoothedGradient->copy( ptrGradient );
      gaussianKernel.ConvolveWithKernel( ptrSmoothedGradient );

      // now determine what the maximal magnitude of the vectors is
      T dMaximalNorm = sqrt( ptrSmoothedGradient->computeMaximalSquareNorm() );

      // and store it
      if ( dMaximalNorm==0 )
      {
        std::cerr << "WARNING: computed a zero gradient; setting gradient scaling factor to 1" << std::endl;
        vecWeights.push_back( 1.0 );
      }
      else
      {
        vecWeights.push_back( dMaximalNorm );
      }

      std::cout << "done. max = " << dMaximalNorm << std::endl;

    }

    // clean up the memory
    delete ptrGradient;
    delete ptrSmoothedGradient;

    return vecWeights;

  }

}

template <class T, unsigned int VImageDimension >
void CMultiGaussianKernel< T, VImageDimension >::ConvolveWithKernel( VectorImageType* pVecImage )
{
  if ( this->m_KernelsNeedToBeComputed )
  {
    if ( m_EstimateGradientScalingFactors )
    {
      m_GradientScalingFactors = ComputeDataDependentScalingFactors();
    }
    ComputeActualWeights();
  }
  Superclass::ConvolveWithKernel( pVecImage );
}

template <class T, unsigned int VImageDimension >
void CMultiGaussianKernel< T, VImageDimension >::ConvolveWithInverseKernel( VectorImageType* pVecImage )
{
  if ( this->m_KernelsNeedToBeComputed )
  {
    if ( m_EstimateGradientScalingFactors )
    {
      m_GradientScalingFactors = ComputeDataDependentScalingFactors();
    }
    ComputeActualWeights();
  }
  Superclass::ConvolveWithInverseKernel( pVecImage );
}

template <class T, unsigned int VImageDimension >
void CMultiGaussianKernel< T, VImageDimension >::ConfirmKernelsNeedToBeComputed()
{
  this->m_KernelsNeedToBeComputed = true;
}

#endif
