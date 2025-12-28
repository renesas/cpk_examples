#include "hal_data.h"
#include "gimp.h"
#include "board_cfg.h"

// #define OSPI_GRAPHICS_OFFSET          (0x90040000)
#define OSPI_GRAPHICS_OFFSET          (0x90001000)

uint32_t get_image_data(st_image_data_t ref);
uint32_t get_sub_image_data(st_image_data_t ref, uint32_t sub_image);
void init_ospi(void);

typedef enum {
    FLASH_TYPE_UNKNOW,
    FLASH_TYPE_AT25SF128A,
    FLASH_TYPE_W25Q128JVPIQ,
    FLASH_TYPE_W25Q128JW
} flash_type_enum;

#define fsp_assert(err)                 \
    do {                                \
        if (err != FSP_SUCCESS) {          \
            printf("fail at %s %d with err %d\n", __func__, __LINE__, err); \
            __BKPT(0);  \
        }               \
    } while (0)

void ospi_test_wait_until_wip(void)
{
    spi_flash_status_t status;
    uint32_t timeout = UINT32_MAX;

    status.write_in_progress = true;

    while ((status.write_in_progress) && (--timeout > 0))
        fsp_assert(R_OSPI_B_StatusGet (g_ospi0.p_ctrl, &status));

    if (0 == timeout)
        fsp_assert (FSP_ERR_TIMEOUT);
}

void init_ospi (void)
{
    flash_type_enum flash_type = FLASH_TYPE_UNKNOW;
    fsp_err_t err = FSP_SUCCESS;
    spi_flash_direct_transfer_t direct_command;

    err = R_OSPI_B_Open(g_ospi0.p_ctrl, g_ospi0.p_cfg);
    fsp_assert (err);

#if 1
    R_XSPI0->LIOCFGCS_b[1].SDRSMPMD = 0x1;

    /* Enable Reset */
    memset(&direct_command, 0, sizeof(direct_command));
    direct_command.command        = 0x66;
    direct_command.command_length = 0x1;
    err = R_OSPI_B_DirectTransfer (g_ospi0.p_ctrl, &direct_command, SPI_FLASH_DIRECT_TRANSFER_DIR_WRITE);
    fsp_assert (err);

    /* Reset Device */
    memset(&direct_command, 0, sizeof(direct_command));
    direct_command.command        = 0x99;
    direct_command.command_length = 0x1;
    err = R_OSPI_B_DirectTransfer (g_ospi0.p_ctrl, &direct_command, SPI_FLASH_DIRECT_TRANSFER_DIR_WRITE);
    fsp_assert (err);

    R_BSP_SoftwareDelay(30, BSP_DELAY_UNITS_MICROSECONDS);

    /* Check Manufacture/Device ID via 1S-1S-1S protocol */
    memset(&direct_command, 0, sizeof(direct_command));
    direct_command.command        = 0x90;
    direct_command.command_length = 0x1;
    direct_command.address        = 0x0000;//0x000000;
    direct_command.address_length = 0x2;//0x3;
    direct_command.dummy_cycles   = 0x8;//0;
    direct_command.data_length    = 0x2;
    err = R_OSPI_B_DirectTransfer (g_ospi0.p_ctrl, &direct_command, SPI_FLASH_DIRECT_TRANSFER_DIR_READ);
    fsp_assert (err);

    if((direct_command.data == 0xEF17) || (direct_command.data == 0x17EF)) {
        flash_type = FLASH_TYPE_W25Q128JVPIQ;
        printf("%s ==> Flash type: W25Q128JVPIQ\n", __FUNCTION__);
    }
    else if((direct_command.data == 0x1F17) || (direct_command.data == 0x171F)) {
        flash_type = FLASH_TYPE_AT25SF128A;
        printf("%s ==> Flash type: AT25SF128A\n", __FUNCTION__);
    }
    else if((direct_command.data == 0xEF18) || (direct_command.data == 0x18EF)) {
        flash_type = FLASH_TYPE_W25Q128JW;
        printf("%s ==> Flash type: W25Q128JW\n", __FUNCTION__);
    }
    else {
        printf("%s ==> Flash type: UNKNOW\n", __FUNCTION__);
    }

#if 0
    if (flash_type == FLASH_TYPE_AT25SF128A) {
        /* Read Status Register Byte 2 */
        memset(&direct_command, 0, sizeof(direct_command));
        direct_command.command        = 0x35;
        direct_command.command_length = 0x1;
        direct_command.address        = 0x0;
        direct_command.address_length = 0x0;
        direct_command.dummy_cycles   = 0x0;
        direct_command.data_length    = 0x1;
        err = R_OSPI_B_DirectTransfer (g_ospi0.p_ctrl, &direct_command, SPI_FLASH_DIRECT_TRANSFER_DIR_READ);
        fsp_assert (err);

        /* Set Write Enable for Volatile Status Register via 1S-1S-1S protocol */
        memset(&direct_command, 0, sizeof(direct_command));
        direct_command.command        = 0x50;
        direct_command.command_length = 0x1;
        err = R_OSPI_B_DirectTransfer (g_ospi0.p_ctrl, &direct_command, SPI_FLASH_DIRECT_TRANSFER_DIR_WRITE);
        fsp_assert (err);
        ospi_test_wait_until_wip();

        /* Set QE bit to 1 to enable Quad */
        memset(&direct_command, 0, sizeof(direct_command));
        direct_command.command        = 0x31;
        direct_command.command_length = 0x1;
        direct_command.data           = 0x02;
        direct_command.data_length    = 0x1;
        err = R_OSPI_B_DirectTransfer (g_ospi0.p_ctrl, &direct_command, SPI_FLASH_DIRECT_TRANSFER_DIR_WRITE);
        fsp_assert (err);

        /* Read Status Register Byte 2 */
        memset(&direct_command, 0, sizeof(direct_command));
        direct_command.command        = 0x35;
        direct_command.command_length = 0x1;
        direct_command.address        = 0x0000;
        direct_command.address_length = 0x0;
        direct_command.dummy_cycles   = 0x0;
        direct_command.data_length    = 0x1;
        err = R_OSPI_B_DirectTransfer (g_ospi0.p_ctrl, &direct_command, SPI_FLASH_DIRECT_TRANSFER_DIR_READ);

        /* Check Manufacture/Device ID via 1S-4S-4S protocol */
        memset(&direct_command, 0, sizeof(direct_command));
        direct_command.command        = 0x94;
        direct_command.command_length = 0x1;
        direct_command.address        = 0x000000;
        direct_command.address_length = 0x3;
        direct_command.dummy_cycles   = 0x6;
        direct_command.data_length    = 0x2;
        err = R_OSPI_B_DirectTransfer (g_ospi0.p_ctrl, &direct_command, SPI_FLASH_DIRECT_TRANSFER_DIR_READ);
        fsp_assert (err);
    }

    err = R_OSPI_B_SpiProtocolSet(g_ospi0.p_ctrl, SPI_FLASH_PROTOCOL_1S_4S_4S);
    fsp_assert (err);
    ospi_test_wait_until_wip();
#endif

    R_BSP_SoftwareDelay(100, BSP_DELAY_UNITS_MICROSECONDS);
#endif
}
