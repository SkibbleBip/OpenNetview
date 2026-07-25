

#include "ISP_init.hpp"
#include "imp/imp_isp.h"
//#include "imp/imp_common.h"
#include <cstring>
#include <iostream>

#define SENSOR_I2C_ADDR 0X40
// magic i2c address for the jxf23 device
#define SENSOR_I2C_NAME (char*)"jxf23"
// name of the device

int ISP_init(void)
{
        int ret = IMP_ISP_Open();

        if(ret != 0){
                std::cerr << "Failed to open ISP: " << ret << std::endl;
        }


        IMPSensorInfo sensor = {};
        sensor.cbus_type = TX_SENSOR_CONTROL_INTERFACE_I2C;
        sensor.i2c.addr = SENSOR_I2C_ADDR;
        sensor.i2c.i2c_adapter = 0;
        /// todo: descrepency between openimp and the proprietary IMP
        /// sensor.i2c.i2c_adapter_id is called sensor.i2c.i2c_adapter in OpenIMP
        strcpy(sensor.name, SENSOR_I2C_NAME);
        strcpy(sensor.i2c.type, SENSOR_I2C_NAME);

        ret = IMP_ISP_AddSensor(&sensor);
        if(ret != 0){
                std::cerr << "Bad sensor: " << ret << " - "<< SENSOR_I2C_NAME << std::endl;
                return -1;
        }
        else{
                std::cout << "Sensor passed: " << SENSOR_I2C_NAME << std::endl;
        }

        ret = IMP_ISP_EnableSensor();
        if(ret != 0){
                std::cerr << "Failed to enable ISP Sensor: " << ret << std::endl;
                return -1;
        }

        ret = IMP_ISP_EnableTuning();
        if(ret != 0){
                std::cerr << "Failed to enable tuning: " << ret << std::endl;
                return -1;
        }



        // log properties
        /*IMPISPSENSORAttr attr = {0};

        ret = IMP_ISP_Tuning_GetSensorAttr(&attr);
        if(ret != 0){
                std::cerr << "Failed to querie Sensor attributes: " << ret << std::endl;
        }
        else{
                std::cout << "Obtained sensor attributes:\n"
                        << "\tFPS: " << attr.fps << "\n"
                        << "\tWidth: " << attr.width << "\n"
                        << "\tHeight: " <<attr.height << std::endl;
        }*/


        std::cout << "Sucessfully init ISP" << std::endl;
        return 0;
}



int ISP_close(void)
{
        int ret = 0;

        ret = IMP_ISP_DisableTuning();
        if( ret != 0 ){
                std::cerr << "Failed to disable tuning: " << ret << std::endl;
        }
        ret = IMP_ISP_DisableSensor();
        if( ret != 0 ){
                std::cerr << "Failed to disable sensor: " << ret << std::endl;
        }
        return 0;
}
