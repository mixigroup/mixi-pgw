//
// Created by mixi on 2017/06/22.
//

#include "mixipgw_mod_diameter.hpp"

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
    {   Diameter xdsrv(argv[1]);
        xdsrv.Start();
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

