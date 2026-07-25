#include <cstdio>
#include <cstdlib>
#include <iostream>
#include "stubs.hpp"

#include "HW_init.hpp"
#include "ISP_init.hpp"
#include "Channel_init.hpp"

#include "imp/imp_system.h"


int main(int argc, char** argv)
{
    std::ios_base::sync_with_stdio(false);
    // fix for broken stdio in the uClib provided by the camera's sysroot

    std::cout << "Starting OpenNetview camera" << std::endl;


        // write the magic config payload to the driver to init the hardware
        int ret = HW_config_init();
        if(ret < 0){
                std::cerr << "Failed to init the hardware driver: " << ret << std::endl;
                return -1;
        }

        ret = IMP_System_Init();

    if (ret < 0) {
        std::cerr << "IMP_System_Init failed: " << ret << std::endl;
        return -1;
    }

    std::cout << "OpenNetview initialised" << std::endl;


    IMPVersion versionInfo = {'\0'};
    ret = IMP_System_GetVersion(&versionInfo);
    if(ret < 0){
        std::cerr << "Failed to get IMP version" << std::endl;
        return -1;
    }
    std::cout << "IMP Version: " << versionInfo.aVersion << std::endl;


        ISP_init();

        Channel_init();


        ISP_close();
        Channel_close();





    IMP_System_Exit();
    return 0;
}
