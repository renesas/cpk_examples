#include "r_mipi_dsi.h"
#include "hal_data.h"
#include "common_utils.h"
#include "common_data.h"
#include "dsi_layer.h"

#define MIPI_DSI_DISPLAY_CONFIG_DATA_DELAY_FLAG      ((mipi_dsi_cmd_id_t) 0xFE)
#define MIPI_DSI_DISPLAY_CONFIG_DATA_END_OF_TABLE    ((mipi_dsi_cmd_id_t) 0xFD)

volatile bool g_message_sent = false;

typedef struct
{
    unsigned char        size;
    unsigned char        buffer[15];
    mipi_dsi_cmd_id_t    cmd_id;
    mipi_dsi_cmd_flag_t flags;
} lcd_table_setting_t;

const lcd_table_setting_t g_lcd_init_focuslcd[] =
{
          // {2,     {0x01, 0x00},                           MIPI_DSI_CMD_ID_DCS_SHORT_WRITE_0_PARAM, MIPI_DSI_CMD_FLAG_LOW_POWER},
          // {120,    {0},                                   MIPI_DSI_DISPLAY_CONFIG_DATA_DELAY_FLAG, (mipi_dsi_cmd_flag_t)0},
          /* enable the BK function of Command2: BK3 */
          /************** BK3 Function start ***************/
//          {2,     {0x11},   MIPI_DSI_CMD_ID_DCS_LONG_WRITE, MIPI_DSI_CMD_FLAG_LOW_POWER},
          //0x11, sleep out,退出休眠
          {2,     {0x11, 0x00},                           MIPI_DSI_CMD_ID_DCS_SHORT_WRITE_0_PARAM, MIPI_DSI_CMD_FLAG_LOW_POWER},

          {2,     {0xF0, 0xC3},                           MIPI_DSI_CMD_ID_DCS_SHORT_WRITE_1_PARAM, MIPI_DSI_CMD_FLAG_LOW_POWER},
          {2,     {0xF0, 0x96},                           MIPI_DSI_CMD_ID_DCS_SHORT_WRITE_1_PARAM, MIPI_DSI_CMD_FLAG_LOW_POWER},
          {2,     {0x36, 0x48},                           MIPI_DSI_CMD_ID_DCS_SHORT_WRITE_1_PARAM, MIPI_DSI_CMD_FLAG_LOW_POWER},
          {2,     {0x3A, 0x55},                           MIPI_DSI_CMD_ID_DCS_SHORT_WRITE_1_PARAM, MIPI_DSI_CMD_FLAG_LOW_POWER},
          {2,     {0xB4, 0x01},                           MIPI_DSI_CMD_ID_DCS_SHORT_WRITE_1_PARAM, MIPI_DSI_CMD_FLAG_LOW_POWER},

          //0xB5, VFP=2,VBP=6, HBP=48
//          {5,     {0xB5, 0x02,0x06,0x00,0x30},                           MIPI_DSI_CMD_ID_DCS_LONG_WRITE, MIPI_DSI_CMD_FLAG_LOW_POWER},

          {4,     {0xB6, 0x8A, 0x07, 0x3B},   MIPI_DSI_CMD_ID_DCS_LONG_WRITE, MIPI_DSI_CMD_FLAG_LOW_POWER},
//          {9,     {0xB6, 0x8A, 0x8A, 0x00,0x00,0x29,0x19,0xA5,0x33},   MIPI_DSI_CMD_ID_DCS_LONG_WRITE, MIPI_DSI_CMD_FLAG_LOW_POWER},
          {2,     {0xB7, 0xC6},                           MIPI_DSI_CMD_ID_DCS_SHORT_WRITE_1_PARAM, MIPI_DSI_CMD_FLAG_LOW_POWER},
          {3,     {0xB9, 0x02,0xE0},                           MIPI_DSI_CMD_ID_DCS_LONG_WRITE, MIPI_DSI_CMD_FLAG_LOW_POWER},
          {3,     {0xC0, 0xC0,0x64},                           MIPI_DSI_CMD_ID_DCS_LONG_WRITE, MIPI_DSI_CMD_FLAG_LOW_POWER},
          {2,     {0xC1, 0x1D},                           MIPI_DSI_CMD_ID_DCS_SHORT_WRITE_1_PARAM, MIPI_DSI_CMD_FLAG_LOW_POWER},
          {2,     {0xC2, 0xA7},                           MIPI_DSI_CMD_ID_DCS_SHORT_WRITE_1_PARAM, MIPI_DSI_CMD_FLAG_LOW_POWER},
          {2,     {0xC5, 0x18},                           MIPI_DSI_CMD_ID_DCS_SHORT_WRITE_1_PARAM, MIPI_DSI_CMD_FLAG_LOW_POWER},
          {9,     {0xE8, 0x40, 0x8A, 0x00,0x00,0x29,0x19,0xA5,0x33},/*0x25,0x0A,0x38,0x33},*//*0x29,0x19,0xA5,0x33},*/   MIPI_DSI_CMD_ID_DCS_LONG_WRITE, MIPI_DSI_CMD_FLAG_LOW_POWER},
          //0xE0, positive gamma 控制
          {15,    {0xE0, 0xF0, 0x0B, 0x12,0x09,0x0A,0x26,0x39,0x54,0x4E,0x38,0x13,0x13,0x2E,0x34},   MIPI_DSI_CMD_ID_DCS_LONG_WRITE, MIPI_DSI_CMD_FLAG_LOW_POWER},
          //0xE1, negative gamma 控制
          {15,    {0xE1, 0xF0, 0x10, 0x15,0x0D,0x0C,0x07,0x38,0x43,0x4D,0x3A,0x16,0x15,0x30,0x35},   MIPI_DSI_CMD_ID_DCS_LONG_WRITE, MIPI_DSI_CMD_FLAG_LOW_POWER},
          {2,     {0xF0, 0x3C},                           MIPI_DSI_CMD_ID_DCS_SHORT_WRITE_1_PARAM, MIPI_DSI_CMD_FLAG_LOW_POWER},
          {2,     {0xF0, 0x69},                           MIPI_DSI_CMD_ID_DCS_SHORT_WRITE_1_PARAM, MIPI_DSI_CMD_FLAG_LOW_POWER},
          {2,     {0x35, 0x00},                           MIPI_DSI_CMD_ID_DCS_SHORT_WRITE_1_PARAM, MIPI_DSI_CMD_FLAG_LOW_POWER},
//          {1,     {0x29},                           MIPI_DSI_CMD_ID_DCS_SHORT_WRITE_1_PARAM, MIPI_DSI_CMD_FLAG_LOW_POWER},
//          {1,     {0x21},                           MIPI_DSI_CMD_ID_DCS_SHORT_WRITE_1_PARAM, MIPI_DSI_CMD_FLAG_LOW_POWER},
          {2,     {0x29, 0x00},                           MIPI_DSI_CMD_ID_DCS_SHORT_WRITE_0_PARAM, MIPI_DSI_CMD_FLAG_LOW_POWER},

          {2,     {0x21, 0x00},                           MIPI_DSI_CMD_ID_DCS_SHORT_WRITE_0_PARAM, MIPI_DSI_CMD_FLAG_LOW_POWER},

          {5,     {0x2A, 0x00, 0x31, 0x01,0x0E},   MIPI_DSI_CMD_ID_DCS_LONG_WRITE, MIPI_DSI_CMD_FLAG_LOW_POWER},
          {5,     {0x2B, 0x00, 0x00, 0x01,0xDF},   MIPI_DSI_CMD_ID_DCS_LONG_WRITE, MIPI_DSI_CMD_FLAG_LOW_POWER},
//          {5,     {0x2B, 0x00, 0x02, 0x01,0xE1},   MIPI_DSI_CMD_ID_DCS_LONG_WRITE, MIPI_DSI_CMD_FLAG_LOW_POWER},

          {2,     {0x2C, 0x00},                           MIPI_DSI_CMD_ID_DCS_SHORT_WRITE_0_PARAM, MIPI_DSI_CMD_FLAG_LOW_POWER},
          {0x00,  {0},                                    MIPI_DSI_DISPLAY_CONFIG_DATA_END_OF_TABLE, (mipi_dsi_cmd_flag_t)0},
};

void mipi_dsi_push_table(const lcd_table_setting_t *table)
{
    const lcd_table_setting_t *p_entry = table;

    while (MIPI_DSI_DISPLAY_CONFIG_DATA_END_OF_TABLE != p_entry->cmd_id)
    {
        mipi_dsi_cmd_t msg =
        {
          .channel = 0,
          .cmd_id = p_entry->cmd_id,
          .flags = p_entry->flags,
          .tx_len = p_entry->size,
          .p_tx_buffer = p_entry->buffer,
        };

        if (MIPI_DSI_DISPLAY_CONFIG_DATA_DELAY_FLAG == msg.cmd_id)
        {
            R_BSP_SoftwareDelay (table->size, BSP_DELAY_UNITS_MILLISECONDS);
        }
        else
        {
            g_message_sent = false;
            /* Send a command to the peripheral device */
            R_MIPI_DSI_Command (&g_mipi_dsi0_ctrl, &msg);
            /* Wait */
            while (!g_message_sent);
        }
        p_entry++;
    }
}

void mipi_dsi0_callback (mipi_dsi_callback_args_t * p_args)
{
    if (MIPI_DSI_EVENT_SEQUENCE_0 == p_args->event)
    {
        g_message_sent = (p_args->tx_status == MIPI_DSI_SEQUENCE_STATUS_DESCRIPTORS_FINISHED);
    }
    else
    {
        /* Put a __BKPT(0) here to check for errors. */
    }
}

void ST7796U_init_HW(void)
{
    R_BSP_PinAccessEnable();
    R_IOPORT_PinWrite(&g_ioport_ctrl, BSP_IO_PORT_00_PIN_00, BSP_IO_LEVEL_HIGH);
    R_BSP_SoftwareDelay(100, BSP_DELAY_UNITS_MILLISECONDS);
    R_IOPORT_PinWrite(&g_ioport_ctrl, BSP_IO_PORT_00_PIN_00, BSP_IO_LEVEL_LOW);
    R_BSP_SoftwareDelay(1, BSP_DELAY_UNITS_MICROSECONDS);
    R_IOPORT_PinWrite(&g_ioport_ctrl, BSP_IO_PORT_00_PIN_00, BSP_IO_LEVEL_HIGH);
    R_BSP_SoftwareDelay(4, BSP_DELAY_UNITS_MILLISECONDS);
    R_IOPORT_PinWrite(&g_ioport_ctrl, BSP_IO_PORT_00_PIN_00, BSP_IO_LEVEL_LOW);
    R_BSP_SoftwareDelay(5, BSP_DELAY_UNITS_MILLISECONDS);
    R_IOPORT_PinWrite(&g_ioport_ctrl, BSP_IO_PORT_00_PIN_00, BSP_IO_LEVEL_HIGH);
}

void mipi_dsi_init(void)
{
    fsp_err_t err = FSP_SUCCESS;

    dsi_layer_enable_backlight();

    ST7796U_init_HW();
   
    /* Initialize GLCDC module */
    err = R_GLCDC_Open(&g_display0_ctrl, &g_display0_cfg);
    if (err != FSP_SUCCESS)
	    return;

    /* Initialize LCD. */
    mipi_dsi_push_table(g_lcd_init_focuslcd);

    err = R_GLCDC_Start(&g_display0_ctrl);
    if (FSP_SUCCESS != err)
        R_GLCDC_Close(&g_display0_ctrl);
}
