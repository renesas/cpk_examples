#include "test.h"

#if TEST_EN_ICM42670

#include "hal_data.h"
#include "perf_counter/perf_counter.h"
#include "utils/log.h"

#define UNLIKE_RETURN(v, t, msg, ...)	if (v != t) { LOG_E(__FUNCTION__, msg, ##__VA_ARGS__); return v; }

static bool s_fifo_irq;
static int s_odr_freq;

uint32_t TestICM42670(inv_imu_device_t *icm_dev)
{
	int rc;
	uint32_t err;
	inv_imu_int1_pin_config_t int1_pin_cfg;
	inv_imu_interrupt_parameter_t int1_source;

	s_fifo_irq = false;

	memset(&int1_pin_cfg, 0x00, sizeof(int1_pin_cfg));
	int1_pin_cfg.int_drive = INT_CONFIG_INT1_DRIVE_CIRCUIT_OD;
	int1_pin_cfg.int_mode = INT_CONFIG_INT1_MODE_PULSED;
	int1_pin_cfg.int_polarity = INT_CONFIG_INT1_POLARITY_LOW;
	rc = inv_imu_set_pin_config_int1(icm_dev, &int1_pin_cfg);
	err = (uint32_t)rc;
	UNLIKE_RETURN(err, 0, "inv_imu_set_pin_config_int1() failed: %d", rc);

	memset(&int1_source, 0x00, sizeof(int1_source));
	int1_source.INV_UI_DRDY = INV_IMU_DISABLE;
	int1_source.INV_FIFO_THS = INV_IMU_ENABLE;
	rc = inv_imu_set_config_int1(icm_dev, &int1_source);
	err = (uint32_t)rc;
	UNLIKE_RETURN(err, 0, "inv_imu_set_config_int1() failed: %d", rc);

	rc = inv_imu_configure_fifo(icm_dev, INV_IMU_FIFO_ENABLED);
	err = (uint32_t)rc;
	UNLIKE_RETURN(err, 0, "inv_imu_configure_fifo() failed: %d", rc);
	rc = inv_imu_reset_fifo(icm_dev);
	err = (uint32_t)rc;
	UNLIKE_RETURN(err, 0, "inv_imu_reset_fifo() failed: %d", rc);
	rc = inv_imu_set_accel_fsr(icm_dev, ACCEL_CONFIG0_FS_SEL_4g);
	err = (uint32_t)rc;
	UNLIKE_RETURN(err, 0, "inv_imu_set_accel_fsr() failed: %d", rc);
	rc = inv_imu_set_gyro_fsr(icm_dev, GYRO_CONFIG0_FS_SEL_2000dps);
	err = (uint32_t)rc;
	UNLIKE_RETURN(err, 0, "inv_imu_set_gyro_fsr() failed: %d", rc);
	if (icm_dev->transport.serif.serif_type == UI_I2C) {
		s_odr_freq = 50;
		rc = inv_imu_set_accel_frequency(icm_dev, ACCEL_CONFIG0_ODR_50_HZ);
		err = (uint32_t)rc;
		UNLIKE_RETURN(err, 0, "inv_imu_set_accel_frequency() failed: %d", rc);
		rc = inv_imu_set_gyro_frequency(icm_dev, GYRO_CONFIG0_ODR_50_HZ);
		err = (uint32_t)rc;
		UNLIKE_RETURN(err, 0, "inv_imu_set_gyro_frequency() failed: %d", rc);
	}
	else {
		s_odr_freq = 50;
		rc = inv_imu_set_accel_frequency(icm_dev, ACCEL_CONFIG0_ODR_50_HZ);
		err = (uint32_t)rc;
		UNLIKE_RETURN(err, 0, "inv_imu_set_accel_frequency() failed: %d", rc);
		rc = inv_imu_set_gyro_frequency(icm_dev, GYRO_CONFIG0_ODR_50_HZ);
		err = (uint32_t)rc;
		UNLIKE_RETURN(err, 0, "inv_imu_set_gyro_frequency() failed: %d", rc);
	}
	rc = inv_imu_enable_accel_low_noise_mode(icm_dev);
	err = (uint32_t)rc;
	UNLIKE_RETURN(err, 0, "inv_imu_enable_accel_low_noise_mode() failed: %d", rc);
	rc = inv_imu_enable_gyro_low_noise_mode(icm_dev);
	err = (uint32_t)rc;
	UNLIKE_RETURN(err, 0, "inv_imu_enable_gyro_low_noise_mode() failed: %d", rc);

	rc = inv_imu_get_data_from_registers(icm_dev);
	err = (uint32_t)rc;
	UNLIKE_RETURN(err, 0, "inv_imu_get_data_from_registers() failed: %d", rc);

	R_ICU_ExternalIrqEnable(g_external_irq31.p_ctrl);

	while (1) {
		while (s_fifo_irq == false) {
			R_BSP_SoftwareDelay(10, BSP_DELAY_UNITS_MICROSECONDS);
		}
		s_fifo_irq = false;

		rc = inv_imu_get_data_from_fifo(icm_dev);
		if (rc < 0) {
			LOG_E(__FUNCTION__, "inv_imu_get_data_from_fifo failed(): %d", rc);
			return (uint32_t)rc;
		}
		else if (rc == 0) {
			LOG_W(__FUNCTION__, "Read FIFO th is 0");
		}
	}

	return 0;
}

void TestICMCallback(inv_imu_sensor_event_t *event)
{
	static int cnt = 0;
	static int accel[3];
	static int gyro[3];
	static int temperature;

	int i;
	double accel_g[3];
	double accel_ms2[3];
	double gyro_dps[3];
	double gyro_rads[3];
	double tempe;

	const double accel_scale_g = 4.0 / 32768.0;
	const double gyro_scale_dps = 2000.0 / 32768.0;
	const double gravity_ms2 = 9.80665;
	const double deg_to_rag = 0.01745329252;

	cnt++;
	for (i = 0; i < 3; i++) {
		accel[i] += event->accel[i];
		gyro[i] += event->gyro[i];
	}
	temperature += event->temperature;

	if (cnt == s_odr_freq) {
		cnt = 0;
		LOG_D(__FUNCTION__, "Get:");

		printf("Original data: 50 times average\r\n");
		for (i = 0; i < 3; i++) {
			accel[i] = accel[i] / s_odr_freq;
			gyro[i] = gyro[i] / s_odr_freq;
		}
		temperature = temperature / s_odr_freq;
		printf("Acceleration: X = %d, Y = %d, Z = %d\r\n", accel[0], accel[1], accel[2]);
		printf("Gyroscope:    X = %d, Y = %d, Z = %d\r\n", gyro[0], gyro[1], gyro[2]);
		printf("Sensor Temperature: %d\r\n", temperature);

		printf("Conversion data:\r\n");
		for (i = 0; i < 3; i++) {
			accel_g[i] = (double)accel[i] * accel_scale_g;
			accel_ms2[i] = (double)accel_g[i] * gravity_ms2;
			gyro_dps[i] = (double)gyro[i] * gyro_scale_dps;
			gyro_rads[i] = (double)gyro_dps[i] * deg_to_rag;
		}
		tempe = 25.0 + (double)temperature / 2.0;
		printf("Acceleration: X = %.4f, Y = %.4f, Z = %.4f m/s^2\r\n", accel_ms2[0], accel_ms2[1], accel_ms2[2]);
		printf("Gyroscope:    X = %.4f, Y = %.4f, Z = %.4f dps\r\n", gyro_dps[0], gyro_dps[1], gyro_dps[2]);
		printf("Gyroscope:    X = %.4f, Y = %.4f, Z = %.4f rad/s\r\n", gyro_rads[0], gyro_rads[1], gyro_rads[2]);
		printf("Sensor Temperature: %.4f ℃\r\n", tempe);

		memset(accel, 0x00, sizeof(accel));
		memset(gyro, 0x00, sizeof(gyro));
		temperature = 0;
	}
}

void ExternalIRQ31_Callback(external_irq_callback_args_t *p_args)
{
	(void)p_args;

	s_fifo_irq = true;
}

#endif
