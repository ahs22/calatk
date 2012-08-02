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

/**
  * Tests the behavior of CApplication class.
  */

#include <cstdlib>
#include "CApplication.h"

int calatkApplicationDataConfigTest2( int argc, char ** argv )
{
  CALATK::CApplication::Pointer calatkApplication;
  try
    {
    calatkApplication = new CALATK::CApplication( argc, argv );
    calatkApplication->Solve();
    }
  catch( const std::exception & e )
    {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
    }

  return EXIT_SUCCESS;
}
