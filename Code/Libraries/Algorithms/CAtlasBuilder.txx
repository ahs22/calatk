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

#ifndef C_ATLAS_BUILDER_TXX
#define C_ATLAS_BUILDER_TXX

template < class TIndividualState, class TState >
CAtlasBuilder< TIndividualState, TState >::CAtlasBuilder()
  : DefaultAtlasIsSourceImage( true ),
    m_ExternallySetAtlasIsSourceImage( false )
{
  m_AtlasIsSourceImage = DefaultAtlasIsSourceImage;
}

template < class TIndividualState, class TState >
CAtlasBuilder< TIndividualState, TState >::~CAtlasBuilder()
{
}

template < class TIndividualState, class TState >
void CAtlasBuilder< TIndividualState, TState >::SetAutoConfiguration( Json::Value& ConfValueIn, Json::Value& ConfValueOut )
{
  Superclass::SetAutoConfiguration( ConfValueIn, ConfValueOut );
  Json::Value& currentConfigurationIn = this->m_jsonConfigIn.GetFromKey( "AtlasSettings", Json::nullValue );
  Json::Value& currentConfigurationOut = this->m_jsonConfigOut.GetFromKey( "AtlasSettings", Json::nullValue );

  SetJSONHelpForRootKey( AtlasSettings, "general settings for the atlas-builder" );

  SetJSONFromKeyBool( currentConfigurationIn, currentConfigurationOut, AtlasIsSourceImage );

  SetJSONHelpForKey( currentConfigurationIn, currentConfigurationOut, AtlasIsSourceImage,
                     "if true the atlas is used as the source image for the registrations, otherwise it will be the target image" );

}

template < class TIndividualState, class TState >
void CAtlasBuilder< TIndividualState, TState >::SetDefaultObjectiveFunctionPointer()
{
  // make sure that all we need has already been allocated

  if ( this->m_ptrKernel.GetPointer() == NULL )
    {
    throw std::runtime_error( "Kernel needs to be defined before default objective function can be created." );
    }

  if ( this->m_ptrMetric.GetPointer() == NULL )
    {
    throw std::runtime_error( "Metric needs to be defined before default objective function can be created." );
    }

  if ( this->m_ptrImageManager.GetPointer() == NULL )
    {
    throw std::runtime_error( "Image manager needs to be defined before default objective function can be created." );
    }

  typedef CAtlasObjectiveFunction< TState > CAtlasType;
  CAtlasType* pAtlas = new CAtlasType;
  this->m_ptrObjectiveFunction = pAtlas;
  this->m_ptrKernel->SetObjectiveFunction( pAtlas );
  pAtlas->SetEvolverPointer( this->m_ptrEvolver );
  pAtlas->SetKernelPointer( this->m_ptrKernel );
  pAtlas->SetMetricPointer( this->m_ptrMetric );
  pAtlas->SetImageManagerPointer( this->m_ptrImageManager );

  // TODO: have an alternative method where the image is part of the gradient

  // TODO: Implement two options, one with atlas image as source and one with the atlas-image as target image

  // TODO: Make the objective functions aware of what they should look at (a particular subject id, everything, ...)

  // TODO: put in sanity check that makes sure that only one time-point is specified for each of the subjects (here for the atlas-building)
  // This time-point is then ignored

  // TODO: TO be able to run this multi-threaded we will likely need to have multiple instances of the kernel object, also need to check that all the other algorithm components are thread-safe!!

}

template < class TIndividualState, class TState >
void CAtlasBuilder< TIndividualState, TState >::Solve()
{
  // TODO: create the atlas-entries for the average image for the time-series

  // copy the input image manager (TODO: need for a proper copy constructor?)

  Superclass::Solve();
}

template < class TIndividualState, class TState >
void CAtlasBuilder< TIndividualState, TState >::PreSubIterationSolve()
{
  // replace the source / target image by its weighted average

  if ( m_AtlasIsSourceImage ) // then atlas is source image (time-point 0)
  {

  }
  else // then atlas is target image (time-point 1)
  {

  }

}

#endif


