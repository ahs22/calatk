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

#ifndef C_SOLVER_MULTI_SCALE_TXX
#define C_SOLVER_MULTI_SCALE_TXX

template < class TState >
CSolverMultiScale< TState >::CSolverMultiScale()
  : DefaultSingleScaleSolver( "LineSearchUnconstrained" ),
    m_ExternallySetSingleScaleSolver( false ),
    DefaultNumberOfSubIterations( 1 ),
    m_ExternallySetNumberOfSubIterations( false )
{
  m_SingleScaleSolver = DefaultSingleScaleSolver;
  m_NumberOfSubIterations = DefaultNumberOfSubIterations;
  this->m_ptrSolver = NULL;
}

template < class TState >
CSolverMultiScale< TState >::~CSolverMultiScale()
{
}

template < class TState >
void CSolverMultiScale< TState >::SetAutoConfiguration( Json::Value &ConfValueIn, Json::Value &ConfValueOut )
{
  Superclass::SetAutoConfiguration( ConfValueIn, ConfValueOut );
  Json::Value& currentConfigurationIn = this->m_jsonConfigIn.GetFromKey( "MultiScaleFinalOutput", Json::nullValue );
  Json::Value& currentConfigurationOut = this->m_jsonConfigOut.GetFromKey( "MultiScaleFinalOutput", Json::nullValue );

  SetJSONHelpForRootKey( MultiScaleFinalOutput, "output after the last stage of the multi-scale solver");

  SetJSONFromKeyBool( currentConfigurationIn, currentConfigurationOut, OutputStateInformation );
  SetJSONFromKeyUInt( currentConfigurationIn, currentConfigurationOut, OutputStateInformationFrequency );

  SetJSONHelpForKey( currentConfigurationIn, currentConfigurationOut, OutputStateInformation,
                     "if set to true will generate output images" );
  SetJSONHelpForKey( currentConfigurationIn, currentConfigurationOut, OutputStateInformationFrequency,
                     "at what iteration steps output should be generated" );

  Json::Value& currentConfigurationInGS = this->m_jsonConfigIn.GetFromKey( "MultiScaleGeneralSettings", Json::nullValue );
  Json::Value& currentConfigurationOutGS = this->m_jsonConfigOut.GetFromKey( "MultiScaleGeneralSettings", Json::nullValue );

  SetJSONHelpForRootKey( MultiScaleGeneralSettings, "general settings, affecting each scale of the multiscale soltion" );

  SetJSONFromKeyString( currentConfigurationInGS, currentConfigurationOutGS, SingleScaleSolver );
  SetJSONFromKeyUInt( currentConfigurationInGS, currentConfigurationOutGS, NumberOfSubIterations );

  SetJSONHelpForKey( currentConfigurationInGS, currentConfigurationOutGS, SingleScaleSolver,
                     "specifies the numerical solver: IpOpt, LineSearchUnconstrained, LineSearchConstrained, NLOpt, LBFGS; IpOpt or LineSearchUnconstrained are recommended. All but LineSearch use an L-BFGS quasi Newton method.");
  SetJSONHelpForKey( currentConfigurationInGS, currentConfigurationOutGS, NumberOfSubIterations,
                     "specified how many times the solver should be called (per scale) while applying a pre-condition")

}

template < class TState >
void CSolverMultiScale< TState >::SetDefaultSingleScaleSolver()
{
  this->m_ptrSolver = CSolverFactory< TState >::CreateNewSolver( m_SingleScaleSolver );
}

template < class TState >
void CSolverMultiScale< TState >::SetSingleScaleSolverPointer( Superclass* ptrSolver )
{
  this->m_ptrSolver = ptrSolver;
}

template < class TState >
const typename CSolverMultiScale< TState >::Superclass*
CSolverMultiScale< TState >::GetSingleScaleSolverPointer() const
{
  return this->m_ptrSolver.GetPointer();
}

template < class TState >
bool CSolverMultiScale< TState >::SolvePreInitialized()
{
  // there is not pre-initialization here necessary (because this is the multi-scale solver), so just call solve
  return this->Solve();
}

template < class TState >
void CSolverMultiScale< TState >::PreSubIterationSolve()
{
  // TODO: Put functionality here. Should call an exteranlly specified method (maybe point to algorithm and have a PreSubIterationSolve method there)
}

template < class TState >
bool CSolverMultiScale< TState >::Solve()
{
  bool reducedEnergy = false;

  // get the objective function which should be minimized and holds the data
  ObjectiveFunctionType * objectiveFunction = this->GetObjectiveFunction();

  assert( objectiveFunction != NULL );

  if ( m_ptrSolver.GetPointer() == NULL )
    {
    SetDefaultSingleScaleSolver();
    }

  assert( m_ptrSolver.GetPointer() != NULL );

  this->m_ptrSolver->SetObjectiveFunction( this->GetObjectiveFunction() );

  // get it's image manager
  ImageManagerMultiScaleType* ptrImageManager = dynamic_cast< ImageManagerMultiScaleType* >( objectiveFunction->GetImageManagerPointer() );

  if ( !ptrImageManager->SupportsMultiScaling() )
    {
    throw std::runtime_error( "Image manager needs to support multi-scaling to use the multi-scale solver.");
    }

  // find all the scales from the image manager
  unsigned int uiNrOfScales = ptrImageManager->GetNumberOfScales();

  assert( uiNrOfScales>0 );

  std::string sSolutionPrefix = "MS-Sol-";

  bool MultiScaleHasBeenInitialized = false;

  Json::Value& currentConfigurationIn = this->m_jsonConfigIn.GetFromKey( "MultiscaleSettings", Json::nullValue );
  Json::Value& currentConfigurationOut = this->m_jsonConfigOut.GetFromKey( "MultiscaleSettings", Json::nullValue );

  SetJSONHelpForRootKey( MultiscaleSettings, "settings for the multiscale solver" );

  // loop over all scales, starting at the lowest
  for ( int iI=(int)uiNrOfScales-1; iI>=0; --iI )
    {
    ptrImageManager->SelectScale( (unsigned int)iI );
    m_ptrSolver->SetExternalSolverState( (unsigned int)iI );
    
    m_ptrSolver->SetPrintConfiguration( this->GetPrintConfiguration() );
    m_ptrSolver->SetAllowHelpComments( this->GetAllowHelpComments() );
    m_ptrSolver->SetAutoConfiguration(
          this->m_jsonConfigIn.GetFromIndex( currentConfigurationIn, iI, Json::nullValue ),
          this->m_jsonConfigOut.GetFromIndex( currentConfigurationOut, iI, Json::nullValue )
          );

    if ( !MultiScaleHasBeenInitialized )
      {
        for ( unsigned int iS=0; iS<m_NumberOfSubIterations; ++iS )
        {
          this->PreSubIterationSolve();

          std::cout << "Solving multiscale level " << iI+1 << "/" << uiNrOfScales << "; subiteration " << iS+1 << "/" << m_NumberOfSubIterations << std::endl;
          if ( iS==0 )
          {
            std::cout << "Initializing multi-scale solution." << std::endl;
            reducedEnergy = reducedEnergy || m_ptrSolver->Solve();
          }
          else
          {
            reducedEnergy = reducedEnergy || m_ptrSolver->SolvePreInitialized();
          }
        }
      MultiScaleHasBeenInitialized = true;
      }
    else // MultiScaleHasBeenInitialized (i.e., this is not the initial scale)
      {
        // has solution from previous iteration
        // get state, upsample it and then use if for initialization
        const TState* pCurrentState = objectiveFunction->GetStatePointer();

        std::cout << "Upsampling state for multi-scale solver." << std::endl;

        typename TState::Pointer pUpsampledState = dynamic_cast< TState* >( pCurrentState->CreateUpsampledStateAndAllocateMemory( ptrImageManager->GetGraftImagePointer() ) );
      
        objectiveFunction->InitializeState( pUpsampledState );

        for ( unsigned int iS=0; iS<m_NumberOfSubIterations; ++iS )
          {

          this->PreSubIterationSolve();
          std::cout << "Solving multiscale level " << iI+1 << "/" << uiNrOfScales << "; subiteration " << iS+1 << "/" << m_NumberOfSubIterations << std::endl;

          reducedEnergy = reducedEnergy || m_ptrSolver->SolvePreInitialized();

          }
      } // end MultiScaleHaseBeenInitialized

      // output the solution at this level
      if ( this->m_OutputStateInformation )
        {
        objectiveFunction->OutputStateInformation( iI, sSolutionPrefix );
        }

    } // loop over scales
  
  return reducedEnergy;
}

#endif
