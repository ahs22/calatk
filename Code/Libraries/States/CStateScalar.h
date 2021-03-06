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

#ifndef C_STATE_SCALAR_H
#define C_STATE_SCALAR_H

#include "CState.h"

namespace CALATK
{

/**
 * \class CStateScalar
 *
 * \brief A state representing a single scalar.
 *
 * \todo more detailed description
 */
template < class TScalar >
class CStateScalar : public CState< TScalar >
{
public:
  /* Standard class typedefs. */
  typedef CStateScalar              Self;
  typedef itk::SmartPointer< Self >        Pointer;
  typedef itk::SmartPointer< const Self >  ConstPointer;
  typedef CState< TScalar >                Superclass;

  CStateScalar()
  {
    this->m_Scalar = 0.0;
  }

  // copy constructor, obvious for scalar case, this is just to show the principle
  CStateScalar( const CStateScalar& c ) : m_Scalar( c.m_Scalar ) { }

  // now declare obvious methods, just to demonstrate how
  // it would be done for a more complicated data structure

  // to support assignment
  CStateScalar & operator=( const CStateScalar &p )
  {
    if ( this==&p )
      {
      return *this;
      }
    this->m_Scalar = p.m_Scalar;
    return *this;
  }

  // to support +=
  CStateScalar & operator+=( const CStateScalar &p )
  {
    this->m_Scalar += p.m_Scalar;
    return *this;
  }

  // to support -=
  CStateScalar & operator-=( const CStateScalar &p )
  {
    this->m_Scalar -= p.m_Scalar;
    return *this;
  }

  // to support *=
  CStateScalar & operator*=( const TScalar &p )
  {
    this->m_Scalar *= p;
    return *this;
  }

  // to support addition
  CStateScalar operator+(const CStateScalar &p ) const
  {
    CStateScalar r = *this;
    return r += p;
  }

  // to support subtraction
  CStateScalar operator-(const CStateScalar &p ) const
  {
    CStateScalar r = *this;
    return r -= p;
  }

  // to support right sided multiplication with a scalar
  CStateScalar operator*(const TScalar &p ) const
  {
    CStateScalar r = *this;
    return r *= p;
  }

  inline void SetValue( TScalar val )
  {
    this->m_Scalar = val;
  }

  inline TScalar GetValue()
  {
    return this->m_Scalar;
  }

  inline TScalar SquaredNorm()
  {
    return this->m_Scalar * this->m_Scalar;
  }

private:
  TScalar m_Scalar;
};

// make sure we can output it
template < class TScalar >
std::ostream &operator<<(std::ostream &stream, CStateScalar< TScalar >& o)
{
  stream << o.GetValue();
  return stream;
}

} // end namespace

#endif
