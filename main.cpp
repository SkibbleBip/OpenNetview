#include <cstdio>
#include <cstdlib>
#include <iostream>

#include "config_payload.h"

#include "imp/imp_system.h"


int main(int argc, char** argv)
{
    std::ios_base::sync_with_stdio(false);
    // fix for broken stdio in the uClib provided by the camera's sysroot

    std::cout << "Starting OpenNetview camera" << std::endl;



    int ret = IMP_System_Init();

    if (ret < 0) {
        std::cout << "IMP_System_Init failed: " << ret << std::endl;
        return -1;
    }

    std::cout << "OpenNetview initialised" << std::endl;


    IMPVersion versionInfo = {'\0'};
    ret = IMP_System_GetVersion(&versionInfo);
    if(ret < 0){
        std::cout << "Failed to get IMP version" << std::endl;
        return -1;
    }
    std::cout << "IMP Version: " << versionInfo.aVersion << std::endl;


    IMP_System_Exit();
    return 0;
}
