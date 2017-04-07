/**
* @file GravityModel.hpp
* 
*/

#ifndef POD_GRAVITY_MODEL_H
#define POD_GRAVITY_MODEL_H
#include"OrbitSim.h"
#include "ForceModel.hpp"
#include"ForceModelData.h"

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

        ///default constructor.
       GravityModel(const GravityModelData &gData);

         /// Default destructor
      virtual ~GravityModel() {};

         /** Call the relevant methods to compute the acceleration.
          * @param utc Time reference class
          * @param rb  Reference body class
          * @param sc  Spacecraft parameters and state
          * @return the acceleration [m/s^s]
          */
      virtual void doCompute(Epoch time, Spacecraft& sc) =0;

         /// return the force model index
      virtual int forceIndex() const
      { return FMI_GEOEARTH; }


   protected:

       GravityModelData  gmData_;

   }; // End of namespace 'gpstk'

      // @}

}  // End of namespace 'gpstk'

#endif   // POD_GRAVITY_MODEL_H
