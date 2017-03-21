#ifndef POD_SOLUTION_H
#define POD_SOLUTION_H

#include"stdafx.h"
using namespace gpstk;
namespace POD
{
    class Solution : public BasicFramework
    {
       
    public:
        Solution(char* path);
       virtual ~Solution()
        {
            delete solver;
        }
        virtual void process();

    protected:

        bool loadConfig(char* path);

        CommandOptionWithArg confFile;

        // Configuration file reader
        ConfDataReader confReader;

        PPPSolutionBase* solver;

    };
}

#endif // !POD_SOLUTION_H
