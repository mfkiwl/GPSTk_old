#ifndef POD_FORCE_MODEL_DATA_H
#define POD_FORCE_MODEL_DATA_H

#include"GravityModel.h"
#include<string>

namespace POD
{
    struct ForceModelData
    {
        ForceModelData();
        ~ForceModelData();

        bool useGravEarh = true;
        bool useGravSun = false;
        bool useGravMoon = false;
        bool useAtmDrag = false;
        bool useRelEffect = false;
        bool useSolarPressure = false;

        GravityModelData gData;
    };

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

        int desiredDegree = 0;
        int desiredOrder = 0;

        int maxDegree;
        int maxOrder;

        bool solidTide = false;
        bool oceanTide = false;
        bool poleTide = false;

        //for debug purposes only
        Matrix<double> unnormalizedCS;

        Matrix<double> normalizedCS;

        bool isModelLoaded = false;
        
        void loadModel(const std::string &path);

    };
}

#endif // !POD_FORCE_MODEL_DATA_H
