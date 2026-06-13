/**
 * @file task_distance.c
 * @brief VL53L0X distance sensor interface.
 */

#include "task_distance.h"
#include "usbd_cdc_if.h"

#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include "usb_print.h"

extern I2C_HandleTypeDef hi2c1;

#define VL53L0X_DEFAULT_ADDR  0x52
#define TOF1_ADDR             0x54
#define TOF2_ADDR             0x56

task_distance_cfg task_distance =
{
    .state = 0,
    .runs = 0,
    .num_states = 3,
    .state_list =
    {
        task_distance_state_0,
        task_distance_state_1,
        task_distance_state_2
    }
};

static VL53L0X_Dev_t tof1_dev;
static VL53L0X_Dev_t tof2_dev;

static VL53L0X_DEV tof1 = &tof1_dev;
static VL53L0X_DEV tof2 = &tof2_dev;

static VL53L0X_RangingMeasurementData_t tof1_data;
static VL53L0X_RangingMeasurementData_t tof2_data;




/**
 * @brief Initializes the VL53L0X distance sensor task.
 *
 * Scans the I2C bus, initializes both ToF sensors, and reports startup status
 * through USB debug messages.
 */
void task_distance_initialize(void)
{
	  TOF_I2C_Scan_With_Both_On();

	  USB_Print("Starting VL53L0X init\r\n");
	  HAL_Delay(200);

	  int16_t lidar_status = TOF_InitTwo();

	  if (lidar_status != 0)
	  {
	      USB_Printf("VL53L0X init failed. status = %d\r\n", (int)lidar_status);

	      while (1)
	      {
	          HAL_Delay(1000);
	      }
	  }

	  USB_Print("VL53L0X init done\r\n");


}

static void TOF_Both_XSHUT(GPIO_PinState state)
{
    HAL_GPIO_WritePin(TOF1_XShut_GPIO_Port, TOF1_XShut_Pin, state);
    HAL_GPIO_WritePin(TOF2_XShut_GPIO_Port, TOF2_XShut_Pin, state);
}

/**
 * @brief Performs an I2C scan with both VL53L0X sensors enabled.
 */
void TOF_I2C_Scan_With_Both_On(void)
{
    uint8_t found_count = 0;

    USB_Print("I2C scan: turning both TOF sensors ON\r\n");

    TOF_Both_XSHUT(GPIO_PIN_SET);
    HAL_Delay(250);

    USB_Printf("I2C scan XSHUT readback: TOF1=%d TOF2=%d\r\n",
                   HAL_GPIO_ReadPin(TOF1_XShut_GPIO_Port, TOF1_XShut_Pin),
                   HAL_GPIO_ReadPin(TOF2_XShut_GPIO_Port, TOF2_XShut_Pin));

    USB_Print("I2C scan start\r\n");

    for (uint8_t addr7 = 1; addr7 < 127; addr7++)
    {
        if (HAL_I2C_IsDeviceReady(&hi2c1, (uint16_t)(addr7 << 1), 2, 10) == HAL_OK)
        {
            found_count++;
            USB_Printf("I2C found: 7-bit 0x%02X, HAL addr 0x%02X\r\n",
                           addr7,
                           (unsigned int)(addr7 << 1));
        }
    }

    if (found_count == 0)
    {
    	USB_Print("I2C scan found NO devices\r\n");
    }

    USB_Print("I2C scan done\r\n");

    TOF_Both_XSHUT(GPIO_PIN_RESET);
    HAL_Delay(150);
}

static VL53L0X_Error TOF_Configure(VL53L0X_DEV dev)
{
    VL53L0X_Error status = VL53L0X_ERROR_NONE;
    uint32_t refSpadCount;
    uint8_t isApertureSpads;
    uint8_t VhvSettings;
    uint8_t PhaseCal;

    status = VL53L0X_StaticInit(dev);
    if (status != VL53L0X_ERROR_NONE) return status;

    status = VL53L0X_PerformRefCalibration(dev, &VhvSettings, &PhaseCal);
    if (status != VL53L0X_ERROR_NONE) return status;

    status = VL53L0X_PerformRefSpadManagement(dev, &refSpadCount, &isApertureSpads);
    if (status != VL53L0X_ERROR_NONE) return status;

    status = VL53L0X_SetDeviceMode(dev, VL53L0X_DEVICEMODE_SINGLE_RANGING);
    if (status != VL53L0X_ERROR_NONE) return status;

    status = VL53L0X_SetLimitCheckEnable(dev, VL53L0X_CHECKENABLE_SIGMA_FINAL_RANGE, 1);
    if (status != VL53L0X_ERROR_NONE) return status;

    status = VL53L0X_SetLimitCheckEnable(dev, VL53L0X_CHECKENABLE_SIGNAL_RATE_FINAL_RANGE, 1);
    if (status != VL53L0X_ERROR_NONE) return status;

    status = VL53L0X_SetLimitCheckValue(dev,
                                        VL53L0X_CHECKENABLE_SIGNAL_RATE_FINAL_RANGE,
                                        (FixPoint1616_t)(0.1f * 65536.0f));
    if (status != VL53L0X_ERROR_NONE) return status;

    status = VL53L0X_SetLimitCheckValue(dev,
                                        VL53L0X_CHECKENABLE_SIGMA_FINAL_RANGE,
                                        (FixPoint1616_t)(60.0f * 65536.0f));
    if (status != VL53L0X_ERROR_NONE) return status;

    status = VL53L0X_SetMeasurementTimingBudgetMicroSeconds(dev, 33000);
    if (status != VL53L0X_ERROR_NONE) return status;

    status = VL53L0X_SetVcselPulsePeriod(dev, VL53L0X_VCSEL_PERIOD_PRE_RANGE, 18);
    if (status != VL53L0X_ERROR_NONE) return status;

    status = VL53L0X_SetVcselPulsePeriod(dev, VL53L0X_VCSEL_PERIOD_FINAL_RANGE, 14);
    if (status != VL53L0X_ERROR_NONE) return status;

    return status;
}

static VL53L0X_Error TOF_InitOne(VL53L0X_DEV dev,
                                 const char *name,
                                 GPIO_TypeDef *xshut_port,
                                 uint16_t xshut_pin,
                                 uint8_t new_addr)
{
    VL53L0X_Error status = VL53L0X_ERROR_NONE;

    USB_Printf("%s: starting\r\n", name);

    USB_Printf("%s: setting XSHUT high\r\n", name);
    HAL_GPIO_WritePin(xshut_port, xshut_pin, GPIO_PIN_SET);
    HAL_Delay(150);

    uint8_t xshut_read = (uint8_t)HAL_GPIO_ReadPin(xshut_port, xshut_pin);

    USB_Printf("%s: XSHUT pin readback = %u\r\n",
                   name,
                   (unsigned int)xshut_read);

    if (xshut_read == 0)
    {
        USB_Printf("%s: XSHUT stayed LOW. This is GPIO/pin/hardware, not I2C.\r\n", name);
        return VL53L0X_ERROR_CONTROL_INTERFACE;
    }

    dev->I2cHandle = &hi2c1;
    dev->I2cDevAddr = VL53L0X_DEFAULT_ADDR;

    USB_Printf("%s: checking I2C address 0x52\r\n", name);

    if (HAL_I2C_IsDeviceReady(&hi2c1, VL53L0X_DEFAULT_ADDR, 5, 100) != HAL_OK)
    {
        USB_Printf("%s: NOT FOUND at I2C addr 0x52\r\n", name);
        return VL53L0X_ERROR_CONTROL_INTERFACE;
    }

    USB_Printf("%s: found at 0x52\r\n", name);

    USB_Printf("%s: boot delay\r\n", name);
    HAL_Delay(100);

    USB_Printf("%s: DataInit\r\n", name);
    status = VL53L0X_DataInit(dev);

    if (status != VL53L0X_ERROR_NONE)
    {
        USB_Printf("%s: DataInit failed, status = %d\r\n", name, (int)status);
        return status;
    }

    USB_Printf("%s: setting new addr 0x%02X\r\n", name, new_addr);
    status = VL53L0X_SetDeviceAddress(dev, new_addr);

    if (status != VL53L0X_ERROR_NONE)
    {
        USB_Printf("%s: SetDeviceAddress failed, status = %d\r\n", name, (int)status);
        return status;
    }

    HAL_Delay(10);
    dev->I2cDevAddr = new_addr;

    USB_Printf("%s: checking new addr 0x%02X\r\n", name, new_addr);

    if (HAL_I2C_IsDeviceReady(&hi2c1, new_addr, 5, 100) != HAL_OK)
    {
        USB_Printf("%s: NOT FOUND at new addr 0x%02X\r\n", name, new_addr);
        return VL53L0X_ERROR_CONTROL_INTERFACE;
    }

    USB_Printf("%s: configuring\r\n", name);
    status = TOF_Configure(dev);

    if (status != VL53L0X_ERROR_NONE)
    {
        USB_Printf("%s: TOF_Configure failed, status = %d\r\n", name, (int)status);
        return status;
    }

    USB_Printf("%s: init OK, addr = 0x%02X\r\n", name, new_addr);
    return VL53L0X_ERROR_NONE;
}

/**
 * @brief Initializes both VL53L0X sensors.
 *
 * Uses the XSHUT pins to turn on each sensor individually and assign unique
 * I2C addresses.
 *
 * @return 0 if both sensors initialize successfully, otherwise a VL53L0X error code.
 */
int16_t TOF_InitTwo(void)
{
    VL53L0X_Error status = VL53L0X_ERROR_NONE;

    USB_Print("Entered TOF_InitTwo\r\n");
    USB_Print("Turning both TOF sensors off\r\n");

    TOF_Both_XSHUT(GPIO_PIN_RESET);
    HAL_Delay(150);

    USB_Print("Initializing TOF1\r\n");
    status = TOF_InitOne(tof1, "TOF1", TOF1_XShut_GPIO_Port, TOF1_XShut_Pin, TOF1_ADDR);
    if (status != VL53L0X_ERROR_NONE) return (int16_t)status;

    USB_Print("Initializing TOF2\r\n");
    status = TOF_InitOne(tof2, "TOF2", TOF2_XShut_GPIO_Port, TOF2_XShut_Pin, TOF2_ADDR);
    if (status != VL53L0X_ERROR_NONE) return (int16_t)status;

    USB_Print("Both TOF sensors initialized\r\n");
    return 0;
}

/**
 * @brief Reads distance measurements from both VL53L0X sensors.
 *
 * Performs single ranging measurements and stores distance, status, and error
 * information in the output structures.
 *
 * @param out1 Pointer to the first sensor measurement structure.
 * @param out2 Pointer to the second sensor measurement structure.
 */
void TOF_ReadTwo(TOF_Measurement_t *out1,
                 TOF_Measurement_t *out2)
{
    VL53L0X_Error s1;
    VL53L0X_Error s2;

    s1 = VL53L0X_PerformSingleRangingMeasurement(tof1, &tof1_data);
    s2 = VL53L0X_PerformSingleRangingMeasurement(tof2, &tof2_data);

    if (out1 != NULL)
    {
        out1->distance_mm = tof1_data.RangeMilliMeter;
        out1->range_status = tof1_data.RangeStatus;
        out1->error = (int16_t)s1;
    }

    if (out2 != NULL)
    {
        out2->distance_mm = tof2_data.RangeMilliMeter;
        out2->range_status = tof2_data.RangeStatus;
        out2->error = (int16_t)s2;
    }
}
