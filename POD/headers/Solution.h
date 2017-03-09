#ifndef SOLUTION
#define SOLUTION

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

#endif // !SOLUTION
