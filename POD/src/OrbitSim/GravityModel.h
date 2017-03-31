//============================================================================
//
//  This file is part of GPSTk, the GPS Toolkit.
//
//  The GPSTk is free software; you can redistribute it and/or modify
//  it under the terms of the GNU Lesser General Public License as published
//  by the Free Software Foundation; either version 3.0 of the License, or
//  any later version.
//
//  The GPSTk is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU Lesser General Public License for more details.
//
//  You should have received a copy of the GNU Lesser General Public
//  License along with GPSTk; if not, write to the Free Software Foundation,
//  Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110, USA
//  
//  Copyright 2004, The University of Texas at Austin
//  Wei Yan - Chinese Academy of Sciences . 2009, 2010
//
//============================================================================

//============================================================================
//
//This software developed by Applied Research Laboratories at the University of
//Texas at Austin, under contract to an agency or agencies within the U.S. 
//Department of Defense. The U.S. Government retains all rights to use,
//duplicate, distribute, disclose, or release this software. 
//
//Pursuant to DoD Directive 523024 
//
// DISTRIBUTION STATEMENT A: This software has been approved for public 
//                           release, distribution is unlimited.
//
//=============================================================================

/**
* @file GravityModel.hpp
* 
*/

#ifndef POD_GRAVITY_MODEL_HPP
#define POD_GRAVITY_MODEL_HPP

#include "ForceModel.hpp"
#include "EarthSolidTide.hpp"
#include "EarthOceanTide.hpp"
#include "EarthPoleTide.hpp"

namespace POD
{

      /// @ingroup GeoDynamics 
      //@{


      /** This class computes the body fixed acceleration due to the harmonic 
       *  gravity field of the central body
       */
   class GravityModel : public ForceModel
   {
   public:

         /** Constructor.
          * @param n Desired degree.
          * @param m Desired order.
          */
      GravityModel(int n, int m);


         /// Default destructor
      virtual ~GravityModel() {};


         /// We declare a pure virtual function
      virtual void initialize() {};

      bool loadModel(const std :: string &path);
   
         /** Computes the acceleration due to gravity in m/s^2.
          * @param r ECI position vector.
          * @param E ECI to ECEF transformation matrix.
          * @return ECI acceleration in m/s^2.
          */
      Vector<double> gravity(Vector<double> r, Matrix<double> E);


         /** Computes the partial derivative of gravity with respect to position.
          * @return ECI gravity gradient matrix.
          * @param r ECI position vector.
          * @param E ECI to ECEF transformation matrix.
          */
      Matrix<double> gravityGradient(Vector<double> r, Matrix<double> E);
      

         /** Call the relevant methods to compute the acceleration.
          * @param utc Time reference class
          * @param rb  Reference body class
          * @param sc  Spacecraft parameters and state
          * @return the acceleration [m/s^s]
          */
      virtual void doCompute(Epoch time, EarthBody& rb, Spacecraft& sc);


      GravityModel& setDesiredDegree(const int& n, const int& m)
      { desiredDegree = n; desiredOrder = m; return (*this); }


      /// Methods to enable earth tide correction

      GravityModel& enableSolidTide(bool b = true)
      { correctSolidTide = b; return (*this);}

      GravityModel& enableOceanTide(bool b = true)
      { correctOceanTide = b; return (*this);}
      
      GravityModel& enablePoleTide(bool b = true)
      { correctPoleTide = b; return (*this);}

         /// Return force model name
      virtual std::string modelName() const
      {return "GravityModel";}

         /// return the force model index
      virtual int forceIndex() const
      { return FMI_GEOEARTH; }


      virtual void test();

   protected:

         /** Evaluates the two harmonic functions V and W.
          * @param r ECI position vector.
          * @param E ECI to ECEF transformation matrix.
          */
      void computeVW(Vector<double> r, Matrix<double> E);

         /// Add tides to coefficients 
      void correctCSTides(Epoch t, bool solidFlag = false, bool oceanFlag = false, bool poleFlag = false);

         /// normalized coefficient
      double normFactor(int n, int m);

   protected:

      struct GravityModelData
      {
         std::string modelName;

         double GM;
         double refDistance;

         bool includesPermTide;

         double refMJD;

         double dotC20;
         double dotC21;
         double dotS21;

         int maxDegree;
         int maxOrder;

         Matrix<double> unnormalizedCS;
         Matrix<double> normalizedCS;

      } gmData;

         /// V W  (nmax+3)*(nmax+3)
         /// Harmonic function V and W
      Matrix<double> V, W;

         /// Degree and Order of gravity model desired.
      int desiredDegree, desiredOrder;
         
         /// Flags to indicate earth tides correction
      bool   correctSolidTide;
      bool   correctPoleTide;
      bool   correctOceanTide;

         /// Objects to do earth tides correction
      EarthSolidTide  solidTide;
      EarthPoleTide   poleTide;
      EarthOceanTide  oceanTide;


   }; // End of namespace 'gpstk'

      // @}

}  // End of namespace 'gpstk'

#endif   // POD_GRAVITY_MODEL_HPP
