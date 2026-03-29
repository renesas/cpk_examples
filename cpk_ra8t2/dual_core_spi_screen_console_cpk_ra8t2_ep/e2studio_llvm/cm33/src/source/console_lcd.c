#include "console.h"

#if CONSOLE_CFG_USE_LCD

#include "arm_2d_disp_adapter_0.h"
#include "arm_2d_helper_scene.h"
#include "arm_2d_types.h"
#include "console_box.h"

static arm_2d_scene_t s_console_scene;
static console_box_t s_console_box;
static uint8_t s_input_buffer[256];
static uint8_t s_console_buffer[40 * 30];

static void afterSwitch(arm_2d_scene_t *ptThis);
static void fnDepose(arm_2d_scene_t *ptThis);
static arm_fsm_rt_t fnScene(void *pTarget, const arm_2d_tile_t *ptTile, bool bIsNewFrame);
static void onFrameCPL(arm_2d_scene_t *ptThis);
static void onFrameStart(arm_2d_scene_t *ptThis);
static void onBGComplete(arm_2d_scene_t *ptThis);
static void onBGStart(arm_2d_scene_t *ptThis);
static void onLoad(arm_2d_scene_t *ptThis);

void CONSOLE_LCD_Init(void)
{
	console_box_cfg_t cfg;
	
	memset(&s_console_scene, 0, sizeof(s_console_scene));
	
	s_console_scene.tCanvas = (arm_2d_colour_t){GLCD_COLOR_WHITE};
	s_console_scene.fnScene = fnScene;
	s_console_scene.fnOnLoad = onLoad;
	s_console_scene.fnAfterSwitch = afterSwitch;
	s_console_scene.fnOnBGStart = onBGStart;
	s_console_scene.fnOnBGComplete = onBGComplete;
	s_console_scene.fnOnFrameStart = onFrameStart;
	s_console_scene.fnOnFrameCPL = onFrameCPL;
	s_console_scene.fnDepose = fnDepose;
	s_console_scene.bUseDirtyRegionHelper = false;
	
	cfg.tBoxSize = (arm_2d_size_t){222, 400};
	cfg.pchConsoleBuffer = s_console_buffer;
	cfg.hwConsoleBufferSize = sizeof(s_console_buffer);
	cfg.pchInputBuffer = s_input_buffer;
	cfg.hwInputBufferSize = sizeof(s_input_buffer);
	cfg.tColor = GLCD_COLOR_GREEN;
	cfg.bUseDirtyRegion = true;
	
	console_box_init(&s_console_box, &s_console_scene, &cfg);
	arm_2d_scene_player_append_scenes(&DISP0_ADAPTER, &s_console_scene, 1);
}

void CONSOLE_LCD_Putchar(char ch)
{
	console_box_putchar(&s_console_box, (uint8_t)ch);
}

static void afterSwitch(arm_2d_scene_t *ptThis)
{
	(void)ptThis;
}

static void fnDepose(arm_2d_scene_t *ptThis)
{
	console_box_depose(&s_console_box);
	ptThis->ptPlayer = NULL;
}

static arm_fsm_rt_t fnScene(void *pTarget, const arm_2d_tile_t *ptTile, bool bIsNewFrame)
{
	arm_2d_canvas(ptTile, __top_canvas) {
		arm_2d_align_centre(__top_canvas, 222, 440) {
			draw_round_corner_box(ptTile, &__centre_region, GLCD_COLOR_BLACK, 128);
			console_box_show(&s_console_box, ptTile, &__centre_region, bIsNewFrame, 255);
		};
		arm_lcd_text_set_target_framebuffer((arm_2d_tile_t *)ptTile);
		arm_lcd_text_set_font(&ARM_2D_FONT_16x24.use_as__arm_2d_font_t);
		arm_lcd_text_set_draw_region(NULL);
		arm_lcd_text_set_colour(GLCD_COLOR_RED, GLCD_COLOR_WHITE);
		arm_lcd_text_location(0, 0);
		arm_lcd_puts("LCD Console");
	}
	arm_2d_op_wait_async(NULL);

	return arm_fsm_rt_cpl;
}

static void onFrameCPL(arm_2d_scene_t *ptThis)
{
	(void)ptThis;
}

static void onFrameStart(arm_2d_scene_t *ptThis)
{
	(void)ptThis;
	
	console_box_on_frame_start(&s_console_box);
}

static void onBGComplete(arm_2d_scene_t *ptThis)
{
	(void)ptThis;
}

static void onBGStart(arm_2d_scene_t *ptThis)
{
	(void)ptThis;
}

static void onLoad(arm_2d_scene_t *ptThis)
{
	(void)ptThis;
}

#endif
