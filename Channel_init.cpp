#include "Channel_init.hpp"

#include "imp/imp_framesource.h"
#include "imp/imp_isp.h"
#include <iostream>
#include <cstdint>
#include <cstddef>



int Channel_init(void)
{
        // we need the sensor framerate first
        int ret = 0;
        uint32_t fps_num = 0, fps_den = 0;
        ret = IMP_ISP_Tuning_GetSensorFPS(&fps_num, &fps_den);
        if( ret != 0){
                std::cerr << "Failed to get sensor tuning FPS: " << ret << std::endl;
                return ret;
        }

        std::cout << "NV framerate: " << fps_num << "/" << fps_den << std::endl;


        IMPFSChnAttr channelAttr = {0};
        channelAttr.picWidth = 1920;
        channelAttr.picHeight = 1080;
        channelAttr.pixFmt = PIX_FMT_NV12;
        channelAttr.outFrmRateNum = fps_num;
        channelAttr.outFrmRateDen = fps_den;
        channelAttr.nrVBs = 1;
        channelAttr.type = FS_PHY_CHANNEL;
        // set frame format and framerate

        channelAttr.crop = {0}; // completely disable cropping
        channelAttr.scaler = {0}; // completely disable scalar

        ret = IMP_FrameSource_CreateChn(0, &channelAttr);
        if(ret != 0){
                std::cerr << "Failed to create channel 0: " << ret <<std::endl;
                return ret;
        }

        ret = IMP_FrameSource_EnableChn(0);
        if(ret != 0){
                std::cerr << "Failed to enable channel 0: " << ret << std::endl;
                return ret;
        }
        std::cout << "Sucessfully enabled channel 0" << std::endl;
        return ret;
}


int Channel_close(void)
{
        int ret = ret = IMP_FrameSource_DisableChn(0);
        //disable the channel
        if (ret < 0) {
                std::cerr << "Failed to disable channel 0: " << ret <<std::endl;
                return ret;
        }
        ret = IMP_FrameSource_DestroyChn(0);
        if (ret < 0) {
                std::cerr << "Failed to destroy channel 0: " << ret <<std::endl;
                return ret;
        }
        return 0;
}



