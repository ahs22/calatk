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

#ifndef REGRESSION_TEST_JSON_H
#define REGRESSION_TEST_JSON_H

#include "json/json-forwards.h"
#include "json/json.h"
#include <iostream>
#include <fstream>

// Compare a test JSON file to a baseline JSON file.  Implementation is
// contained within the header for convenience.

/** Return 0 if the test file and baseline file have the same JSON content
 * and 1 otherwise. */
int RegressionTestJSON( const char *testJSONFileName,
                         const char *baselineJSONFileName,
                         bool reportErrors = true,
                         bool verbose = true );

// Recursively compare test and baseline.
int compareJSON( const Json::Value & test, const Json::Value & baseline, bool reportErrors, bool verbose )
{
  int same = 0;
  for( Json::Value::const_iterator baselineIt = baseline.begin(), testIt = test.begin();
       testIt != test.end();
       ++baselineIt, ++testIt )
    {
    if( !((*baselineIt).isNull()) && ((*baselineIt).isArray() || (*baselineIt).isObject()) )
      {
      if( compareJSON( *testIt, *baselineIt, reportErrors, verbose ) )
        {
        same = 1;
        break;
        }
      }
    else
      {
      if( verbose )
        {
        std::cout << "Comparing: " << *testIt << " to " << *baselineIt << std::endl;
        }
      if( (*baselineIt).isNull() )
        {
        if( !(*testIt).isNull() )
          {
          if( reportErrors )
            {
            std::cerr << "The test value was non-null when the baseline value was null." << std::endl;
            }
          same = 1;
          break;
          }
        }
      else if( (*baselineIt).isBool() )
        {
        if( (*baselineIt).asBool() != (*testIt).asBool() )
          {
          if( reportErrors )
            {
            std::cerr << "The test value: " << (*testIt).asBool() << " does not equal the baseline value: " << (*baselineIt).asBool() << std::endl;
            }
          same = 1;
          break;
          }
        }
      else if( (*baselineIt).isIntegral() )
        {
        if( (*baselineIt).asInt() != (*testIt).asInt() )
          {
          if( reportErrors )
            {
            std::cerr << "The test value: " << (*testIt).asInt() << " does not equal the baseline value: " << (*baselineIt).asInt() << std::endl;
            }
          same = 1;
          break;
          }
        }
      else if( (*baselineIt).isDouble() )
        {
        if( (*baselineIt).asDouble() != (*testIt).asDouble() )
          {
          if( reportErrors )
            {
            std::cerr << "The test value: " << (*testIt).asDouble() << " does not equal the baseline value: " << (*baselineIt).asDouble() << std::endl;
            }
          same = 1;
          break;
          }
        }
      else if( (*baselineIt).isString() )
        {
        if( (*baselineIt).asString() != (*testIt).asString() )
          {
          if( reportErrors )
            {
            std::cerr << "The test value: " << (*testIt).asString() << " does not equal the baseline value: " << (*baselineIt).asString() << std::endl;
            }
          same = 1;
          break;
          }
        }
      }
    }
  return same;
}

int RegressionTestJSON( const char *testJSONFileName,
                         const char *baselineJSONFileName,
                         bool reportErrors,
                         bool verbose )
{
  Json::Value testRoot;
  Json::Value baselineRoot;

  Json::Reader reader;
  std::ifstream testFile( testJSONFileName );
  if( !testFile.is_open() )
    {
    std::cerr << "Could not open test file: " << testJSONFileName << std::endl;
    return 1;
    }
  if( !reader.parse( testFile, testRoot ) )
    {
    std::cerr << "Could not parse test file: " << testJSONFileName << std::endl;
    testFile.close();
    return 1;
    }
  testFile.close();
  std::ifstream baselineFile( baselineJSONFileName );
  if( !baselineFile.is_open() )
    {
    std::cerr << "Could not open baseline file: " << baselineJSONFileName << std::endl;
    return 1;
    }
  if( !reader.parse( baselineFile, baselineRoot ) )
    {
    std::cerr << "Could not parse baseline file: " << baselineJSONFileName << std::endl;
    baselineFile.close();
    return 1;
    }
  baselineFile.close();

  return compareJSON( testRoot, baselineRoot, reportErrors, verbose );
}

#endif
