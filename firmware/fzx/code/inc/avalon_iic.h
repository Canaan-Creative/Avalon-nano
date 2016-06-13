/*
 * @brief iic head file
 *
 * @note
 *
 * @par
 */
#ifndef __AVALON_IIC_H_
#define __AVALON_IIC_H_

#define I2C_PORT    0
#define I2C_SCL_PIN 4
#define I2C_SDA_PIN 5

#define I2C_SCL_H   1
#define I2C_SCL_L   0

#define I2C_SDA_H   1
#define I2C_SDA_L   0

void i2c_init(void);
int i2c_readtemp(void);

#endif /* __AVALON_IIC_H_ */
