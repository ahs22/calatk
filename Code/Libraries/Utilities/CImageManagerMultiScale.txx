#ifndef C_IMAGE_MANAGER_MULTI_SCALE_TXX
#define C_IMAGE_MANAGER_MULTI_SCALE_TXX

template <class T, unsigned int VImageDimension, class TSpace >
CImageManagerMultiScale< T, VImageDimension, TSpace >::CImageManagerMultiScale()
  : DefaultSigma( 0.02 ), m_ExternallySetSigma( false )
{
  m_uiCurrentlySelectedScale = 0;
  m_bImagesWereRead = false;
  m_bSetDefaultResampler = false;
  m_ptrResampler = NULL;

  m_Sigma = DefaultSigma;

  // add default scale, the original blurred image at the original resolution
  AddScale( 1.0, 0 );

}

template <class T, unsigned int VImageDimension, class TSpace >
CImageManagerMultiScale< T, VImageDimension, TSpace >::~CImageManagerMultiScale()
{
  DeleteDefaultResampler();
  // Upsampled images will be deleted by the base class if they have been allocated
}

template <class T, unsigned int VImageDimension, class TSpace >
void CImageManagerMultiScale< T, VImageDimension, TSpace >::DeleteDefaultResampler()
{
  if ( m_bSetDefaultResampler )
    {
    if ( m_ptrResampler != NULL ) delete m_ptrResampler;
    m_ptrResampler = NULL;
    m_bSetDefaultResampler = false;
    }
}

template <class T, unsigned int VImageDimension, class TSpace >
void CImageManagerMultiScale< T, VImageDimension, TSpace >::SetResamplerPointer( ResamplerType* ptrResampler )
{
  DeleteDefaultResampler();  
  this->m_ptrResampler = ptrResampler;
}


template <class T, unsigned int VImageDimension, class TSpace >
void CImageManagerMultiScale< T, VImageDimension, TSpace >::SetDefaultResamplerPointer()
{
  DeleteDefaultResampler();
  m_ptrResampler = new CResamplerLinear< T, VImageDimension >;
  m_bSetDefaultResampler = true;
}

template <class T, unsigned int VImageDimension, class TSpace >
void CImageManagerMultiScale< T, VImageDimension, TSpace >::AddScale( T dScale, unsigned int uiScaleIndx )
{

  if ( m_bImagesWereRead )
    {
    std::runtime_error("Scales cannot be changed after images have been read.");
    return;
    }

  if ( uiScaleIndx < 0 )
    {
    std::runtime_error("Negative scale index is not allowed." );
    return;
    }

  if ( (int)m_ScaleVector.size() <= (int)uiScaleIndx )
    {
    // increase size of vector
    for ( int iI=0; iI <= (int)(uiScaleIndx-m_ScaleVector.size()); ++iI )
      {
      m_ScaleVector.push_back( 0 );
      m_ScaleWasSet.push_back( false );
      }
    }

  assert( m_ScaleVector.size() >= uiScaleIndx+1 );
  // now we know that we can access the element
  m_ScaleVector[ uiScaleIndx ] = dScale;
  m_ScaleWasSet[ uiScaleIndx ] = true;
}

template <class T, unsigned int VImageDimension, class TSpace >
void CImageManagerMultiScale< T, VImageDimension, TSpace >::RemoveScale( unsigned int uiScaleIndx )
{

  if ( m_bImagesWereRead )
    {
    std::runtime_error("Scales cannot be changed after images have been read.");
    return;
    }

  if ( !( uiScaleIndx < 0 || uiScaleIndx >= m_ScaleVector.size() ) )
    {
    // valid range, otherwise don't do anything
    m_ScaleVector[ uiScaleIndx ] = 0;
    m_ScaleWasSet[ uiScaleIndx ] = false;

    // if it is the last element, just pop it
    if ( uiScaleIndx == m_ScaleVector.size()-1 )
      {
      m_ScaleVector.pop_back();
      m_ScaleWasSet.pop_back();
      }
    }
}

template <class T, unsigned int VImageDimension, class TSpace >
unsigned int CImageManagerMultiScale< T, VImageDimension, TSpace >::GetNumberOfScales()
{
  // multi-scale levels which contains the image at the original resolution
  return m_ScaleVector.size();
}

template <class T, unsigned int VImageDimension, class TSpace >
void CImageManagerMultiScale< T, VImageDimension, TSpace >::SelectScale( unsigned int uiScaleIndx )
{
  assert( uiScaleIndx>=0 && uiScaleIndx<m_ScaleVector.size() );
  if ( !( uiScaleIndx>=0 && uiScaleIndx<m_ScaleVector.size() ) )
    {
    throw std::runtime_error("Scale selection index out of range.");
    }

  m_uiCurrentlySelectedScale = uiScaleIndx;

}

template <class T, unsigned int VImageDimension, class TSpace >
void CImageManagerMultiScale< T, VImageDimension, TSpace >::SetScale( SImageInformation* pCurrentImInfo )
{
  pCurrentImInfo->pIm = pCurrentImInfo->pImsOfAllScales[ m_uiCurrentlySelectedScale ];
}

template <class T, unsigned int VImageDimension, class TSpace >
void CImageManagerMultiScale< T, VImageDimension, TSpace >::GetImage( SImageInformation* pCurrentImInfo )
{
  // get the image consistent with the scale, if image has not been loaded, load it and create the spatial pyramid
  // if any scales were altered since last time calling this, we need to reload also

  if ( pCurrentImInfo->pImOrig == NULL )
    {
    // load it and all information that comes with it (even transform)
    Superclass::GetImage( pCurrentImInfo );
    }

  // check if the scales have been created, if not, do so
  if ( pCurrentImInfo->pImsOfAllScales.empty() )
    {
    // create all the scales
    unsigned int uiNrOfScales = this->GetNumberOfScales();
    for ( unsigned int iI=0; iI<uiNrOfScales; ++iI )
      {
      if ( m_ScaleWasSet[ iI ] )
        {
        std::cout << "Computing scale " << m_ScaleVector[iI] << " of: " << pCurrentImInfo->sImageFileName << std::endl;
        assert( m_ScaleVector[iI]>0 );
        VectorImageType* ptrResampledImage = VectorImageUtils< T, VImageDimension >::AllocateMemoryForScaledVectorImage( pCurrentImInfo->pImOrig, m_ScaleVector[ iI ] );

        if ( m_ptrResampler == NULL ) SetDefaultResamplerPointer();

        m_ptrResampler->SetSigma( m_Sigma/m_ScaleVector[ iI ] );
        m_ptrResampler->Downsample( pCurrentImInfo->pImOrig, ptrResampledImage );
        pCurrentImInfo->pImsOfAllScales.push_back( ptrResampledImage );
        }
      else
        {
        std::cout << "WARNING: scale " << iI << " was not set." << std::endl;
        pCurrentImInfo->pImsOfAllScales.push_back( NULL );
        }
      }

    }
  
  if ( pCurrentImInfo->pIm != NULL )
    {
    // was read
    SetScale( pCurrentImInfo );
    m_bImagesWereRead = true; // disallow any future change of the images (implement some form of reset functionality)
    }
  else
    {
    // was not read
    std::runtime_error("Could not read file.");
    }

}

#endif
