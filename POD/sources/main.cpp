// myApp.cpp : Defines the entry point for the console application.
//
#include "stdafx.h"
using namespace std;
using namespace POD;
int main(int argc, char *argv[])
{
    Solution sol(argv[1]);
    sol.process();
}