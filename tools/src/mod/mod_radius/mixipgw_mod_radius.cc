//
// Created by mixi on 2017/06/12.
//

#include "mixipgw_mod_radius.hpp"

using namespace MIXIPGW_TOOLS;

//
int main(int argc, char **argv){
    if (argc != 2){
        fprintf(stderr, "invalid arguments.(argc)\n");
        exit(0);
    }
    // init process.
    Module::Init(NULL);
    Logger::Init(basename(argv[0]),NULL,NULL);

    Misc::SetModuleid(10);
    Misc::SetModuleCfg(argv[1]);
    //
    {   RadiusServer xrsrv(argv[1]);
        xrsrv.Start();
        //
        while(!Module::ABORT()){
            usleep(100000);
        }
    }
    // uninitialize
    Logger::Uninit(NULL);
    Module::Uninit(NULL);
    //
    return (0);
}

