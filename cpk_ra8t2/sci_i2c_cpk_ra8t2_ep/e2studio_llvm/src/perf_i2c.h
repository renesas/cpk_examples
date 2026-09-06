/*
 * perf_i2c.h
 *
 *  Created on: Jan 22, 2026
 *      Author: Ran Qingling
 */

#ifndef PERF_I2C_H_
#define PERF_I2C_H_

#define RESET_VALUE             (0x00)

/* Macro for checking if two buffers are equal */
#define BUFF_EQUAL          (0U)
#define SLAVE_TYPE                  "_SLAVE"
#define SCI_TYPE                    "SCI"
/* Buffer size for slave and master data */
#define BUF_LEN             (0x06)


/* Enumerators to identify master event to be processed */
typedef enum e_master
{
    MASTER_READ  = 1U,
    MASTER_WRITE = 2U
}master_transfer_mode_t;

/* Global functions */
fsp_err_t init_i2c_driver(void);
fsp_err_t process_master_WriteRead(void);
void deinit_i2c_driver(void);

#endif /* PERF_I2C_H_ */
