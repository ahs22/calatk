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

#ifndef C_ALGORITHM_BASE_TXX
#define C_ALGORITHM_BASE_TXX

template < class T, unsigned int VImageDimension >
CAlgorithmBase< T, VImageDimension >::CAlgorithmBase()
{
  this->m_ptrMetric = NULL;
  this->m_ptrImageManager = NULL;
  this->m_ptrEvolver = NULL;
  this->m_ptrKernel = NULL;

  this->m_ptrIm = NULL;
  this->m_ptrMap = NULL;
}

template < class T, unsigned int VImageDimension >
CAlgorithmBase< T, VImageDimension >::~CAlgorithmBase()
{
}

template < class T, unsigned int VImageDimension >
void CAlgorithmBase< T, VImageDimension >::SetDefaultsIfNeeded()
{

  if ( m_ptrMetric.GetPointer() == NULL )
    {
    this->SetDefaultMetricPointer();
    }

  this->m_ptrMetric->SetPrintConfiguration( this->GetPrintConfiguration() );
  this->m_ptrMetric->SetAllowHelpComments( this->GetAllowHelpComments() );
  this->m_ptrMetric->SetAutoConfiguration( *this->m_jsonConfigIn.GetRootPointer(), *this->m_jsonConfigOut.GetRootPointer() );

  if ( this->m_ptrEvolver.GetPointer() == NULL )
    {
    SetDefaultEvolverPointer();
    }

  this->m_ptrEvolver->SetPrintConfiguration( this->GetPrintConfiguration() );
  this->m_ptrEvolver->SetAllowHelpComments( this->GetAllowHelpComments() );
  this->m_ptrEvolver->SetAutoConfiguration( *this->m_jsonConfigIn.GetRootPointer(), *this->m_jsonConfigOut.GetRootPointer() );

  if ( this->m_ptrKernel.GetPointer() == NULL )
    {
    SetDefaultKernelPointer();
    }

  this->m_ptrKernel->SetPrintConfiguration( this->GetPrintConfiguration() );
  this->m_ptrKernel->SetAllowHelpComments( this->GetAllowHelpComments() );
  this->m_ptrKernel->SetAutoConfiguration( *this->m_jsonConfigIn.GetRootPointer(), *this->m_jsonConfigOut.GetRootPointer() );

  // get the subject ids
  std::vector< unsigned int > vecSubjectIndices;
  this->m_ptrImageManager->GetAvailableSubjectIndices( vecSubjectIndices );

  this->m_ptrImageManager->SetPrintConfiguration( this->GetPrintConfiguration() );
  this->m_ptrImageManager->SetAllowHelpComments( this->GetAllowHelpComments() );
  this->m_ptrImageManager->SetAutoConfiguration( *this->m_jsonConfigIn.GetRootPointer(), *this->m_jsonConfigOut.GetRootPointer() );

  // also create the memory for the map and the image, so we can use it to return a map and an image at any time
  SImageInformation* pImInfo;
  this->m_ptrImageManager->GetPointerToSubjectImageInformationByIndex( pImInfo, vecSubjectIndices[ 0 ], 0 );

  assert( this->m_ptrIm.GetPointer() == NULL );
  this->m_ptrIm = new VectorImageType( pImInfo->pIm );
  this->m_ptrIm->SetToConstant( 0 );

  assert( m_ptrMap.GetPointer() == NULL );
  this->m_ptrMap = new VectorFieldType( pImInfo->pIm );
  this->m_ptrMap->SetToConstant( 0 );
}

template < class T, unsigned int VImageDimension >
void CAlgorithmBase< T, VImageDimension >::SetImageManagerPointer( ImageManagerType *  ptrImageManager )
{
  this->m_ptrImageManager = ptrImageManager;
}

template < class T, unsigned int VImageDimension >
typename CAlgorithmBase< T, VImageDimension >::ImageManagerType *
CAlgorithmBase< T, VImageDimension >::GetImageManagerPointer()
{
  if ( this->m_ptrImageManager.GetPointer() == NULL )
    {
    this->SetDefaultImageManagerPointer();
    }
  return this->m_ptrImageManager.GetPointer();
}

template < class T, unsigned int VImageDimension >
void CAlgorithmBase< T, VImageDimension >::SetKernelPointer( KernelType * ptrKernel )
{
  this->m_ptrKernel = ptrKernel;
}

template < class T, unsigned int VImageDimension >
typename CAlgorithmBase< T, VImageDimension >::KernelType *
CAlgorithmBase< T, VImageDimension >::GetKernelPointer()
{
  if ( this->m_ptrKernel.GetPointer() == NULL )
    {
    this->SetDefaultKernelPointer();
    }
  return m_ptrKernel.GetPointer();
}

template < class T, unsigned int VImageDimension >
void CAlgorithmBase< T, VImageDimension >::SetEvolverPointer( EvolverType * ptrEvolver )
{
  this->m_ptrEvolver = ptrEvolver;
}

template < class T, unsigned int VImageDimension >
typename CAlgorithmBase< T, VImageDimension >::EvolverType *
CAlgorithmBase< T, VImageDimension >::GetEvolverPointer()
{
  if ( this->m_ptrEvolver.GetPointer() == NULL )
    {
    this->SetDefaultEvolverPointer();
    }
  return this->m_ptrEvolver.GetPointer();
}


template < class T, unsigned int VImageDimension >
void CAlgorithmBase< T, VImageDimension >::SetMetricPointer( MetricType * ptrMetric )
{
  this->m_ptrMetric = ptrMetric;
}

template < class T, unsigned int VImageDimension >
typename CAlgorithmBase< T, VImageDimension >::MetricType *
CAlgorithmBase< T, VImageDimension >::GetMetricPointer()
{
  if ( this->m_ptrMetric.GetPointer() == NULL )
    {
    this->SetDefaultMetricPointer();
    }
  return this->m_ptrMetric.GetPointer();
}

#endif
