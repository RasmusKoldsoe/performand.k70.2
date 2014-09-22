////////////////////////////////////////////////////////////////////////////
//
//  This file is part of linux-mpu9150
//
//  Copyright (c) 2013 Pansenti, LLC
//
//  Permission is hereby granted, free of charge, to any person obtaining a copy of 
//  this software and associated documentation files (the "Software"), to deal in 
//  the Software without restriction, including without limitation the rights to use, 
//  copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the 
//  Software, and to permit persons to whom the Software is furnished to do so, 
//  subject to the following conditions:
//
//  The above copyright notice and this permission notice shall be included in all 
//  copies or substantial portions of the Software.
//
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, 
//  INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A 
//  PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT 
//  HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION 
//  OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE 
//  SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#include <stdio.h>
#include <string.h>

#include "../glue/linux_glue.h"
#include "../eMPL/inv_mpu.h"
#include "../eMPL/inv_mpu_dmp_motion_driver.h"
#include "mpu9150.h"

//static int data_ready();
static void calibrate_data(mpudata_t *mpu);
static void tilt_compensate(quaternion_t magQ, quaternion_t unfusedQ);
static int data_fusion(mpudata_t *mpu);
static unsigned short inv_row_2_scale(const signed char *row);
static unsigned short inv_orientation_matrix_to_scalar(const signed char *mtx);

int debug_on = 1;
int yaw_mixing_factor;

int use_accel_cal;
caldata_t accel_cal_data;

int use_mag_cal;
caldata_t mag_cal_data;

void mpu9150_set_debug(int on)
{
	debug_on = on;
}

int mpu9150_init(int i2c_bus, int sample_rate, int mix_factor)
{
	signed char gyro_orientation[9] = { 1, 0, 0,
                                        0, 1, 0,
                                        0, 0, 1 };

	if (i2c_bus < MIN_I2C_BUS || i2c_bus > MAX_I2C_BUS) {
		fprintf(stderr, "[IMU_daemon] Invalid I2C bus %d\n", i2c_bus);
		return -1;
	}

	if (sample_rate < MIN_SAMPLE_RATE || sample_rate > MAX_SAMPLE_RATE) {
		fprintf(stderr, "[IMU_daemon] Invalid sample rate %d\n", sample_rate);
		return -1;
	}

	if (mix_factor < 0 || mix_factor > 100) {
		fprintf(stderr, "[IMU_daemon] Invalid mag mixing factor %d\n", mix_factor);
		return -1;
	}

	yaw_mixing_factor = mix_factor;
	linux_set_i2c_bus(i2c_bus);

	fprintf(stderr, "\nInitializing IMU .");
	fflush(stderr);

	if (mpu_init(NULL)) {
		fprintf(stderr, "\n[IMU_daemon] ERROR: mpu_init() failed\n");
		return -1;
	}

	fprintf(stderr, ".");
	fflush(stderr);

	if (mpu_set_sensors(INV_XYZ_GYRO | INV_XYZ_ACCEL | INV_XYZ_COMPASS)) {
		fprintf(stderr, "\n[IMU_daemon} ERROR mpu_set_sensors() failed\n");
		return -1;
	}

	fprintf(stderr, ".");
	fflush(stderr);

	if (mpu_configure_fifo(INV_XYZ_GYRO | INV_XYZ_ACCEL)) {
		fprintf(stderr, "\n[IMU_daemon] ERROR mpu_configure_fifo() failed\n");
		return -1;
	}

	fprintf(stderr, ".");
	fflush(stderr);
	
	if (mpu_set_sample_rate(sample_rate)) {
		fprintf(stderr, "\n[IMU_daemon] ERROR mpu_set_sample_rate() failed\n");
		return -1;
	}

	fprintf(stderr, ".");
	fflush(stderr);

	if (mpu_set_compass_sample_rate(sample_rate)) {
		fprintf(stderr, "\n[IMU_daemon] ERROR mpu_set_compass_sample_rate() failed\n");
		return -1;
	}
/**** 
	fprintf(stderr, ".");
	fflush(stderr);

	if (dmp_load_motion_driver_firmware()) {
		fprintf(stderr, "\n[IMU_daemon] ERROR dmp_load_motion_driver_firmware() failed\n");
		return -1;
	}

	fprintf(stderr, ".");
	fflush(stderr);

	if (dmp_set_orientation(inv_orientation_matrix_to_scalar(gyro_orientation))) {
		fprintf(stderr, "\n[IMU_daemon] ERROR dmp_set_orientation() failed\n");
		return -1;
	}

	fprintf(stderr, ".");
	fflush(stderr);

  	if (dmp_enable_feature(DMP_FEATURE_6X_LP_QUAT | DMP_FEATURE_SEND_RAW_ACCEL 
						| DMP_FEATURE_SEND_CAL_GYRO | DMP_FEATURE_GYRO_CAL)) {
		fprintf(stderr, "\n[IMU_daemon] ERROR dmp_enable_feature() failed\n");
		return -1;
	}

	fprintf(stderr, ".");
	fflush(stderr);
 
	if (dmp_set_fifo_rate(sample_rate)) {
		fprintf(stderr, "\n[IMU_daemon] ERROR dmp_set_fifo_rate() failed\n");
		return -1;
	}

	fprintf(stderr, ".");
	fflush(stderr);

	if (mpu_set_dmp_state(1)) {
		fprintf(stderr, "\n[IMU_daemon] ERROR mpu_set_dmp_state(1) failed\n");
		return -1;
	}
*/
	fprintf(stderr, " done\n");

	return 0;
}

void mpu9150_exit()
{
	// turn off the DMP on exit 
	if (mpu_set_dmp_state(0))
		fprintf(stderr, "mpu_set_dmp_state(0) failed\n");

	// TODO: Should turn off the sensors too
}

void mpu9150_set_config(double *val) {

}


void mpu9150_set_accel_cal(caldata_t *cal)
{
	int i;
	long bias[3];

	if (!cal) {
		use_accel_cal = 0;
		return;
	}

	memcpy(&accel_cal_data, cal, sizeof(caldata_t));

	for (i = 0; i < 3; i++) {
		if (accel_cal_data.range[i] < 1)
			accel_cal_data.range[i] = 1;
		else if (accel_cal_data.range[i] > ACCEL_SENSOR_RANGE)
			accel_cal_data.range[i] = ACCEL_SENSOR_RANGE;

		bias[i] = -accel_cal_data.offset[i];
	}

	if (1) {
		printf("\naccel cal (range : offset)\n");

		for (i = 0; i < 3; i++)
			printf("%0.4f : %0.4f\n", accel_cal_data.range[i], accel_cal_data.offset[i]);
	}

	mpu_set_accel_bias(bias);

	use_accel_cal = 1;
}

void mpu9150_set_mag_cal(caldata_t *cal)
{
	int i;

	if (!cal) {
		use_mag_cal = 0;
		return;
	}

	memcpy(&mag_cal_data, cal, sizeof(caldata_t));

/*	for (i = 0; i < 3; i++) {
		if (mag_cal_data.range[i] < 1)
			mag_cal_data.range[i] = 1;
		else if (mag_cal_data.range[i] > MAG_SENSOR_RANGE)
			mag_cal_data.range[i] = MAG_SENSOR_RANGE;

		if (mag_cal_data.offset[i] < -MAG_SENSOR_RANGE)
			mag_cal_data.offset[i] = -MAG_SENSOR_RANGE;
		else if (mag_cal_data.offset[i] > MAG_SENSOR_RANGE)
			mag_cal_data.offset[i] = MAG_SENSOR_RANGE;
	}

	printf("Mag cal matrix:\n");
	printf("%0.5f,%0.5f,%0.5f\n",mag_cal_data.comp[0],mag_cal_data.comp[1],mag_cal_data.comp[2]);
	printf("%0.5f,%0.5f,%0.5f\n",mag_cal_data.comp[3],mag_cal_data.comp[4],mag_cal_data.comp[5]);
	printf("%0.5f,%0.5f,%0.5f\n",mag_cal_data.comp[6],mag_cal_data.comp[7],mag_cal_data.comp[8]);
	printf("Mag offset vector:\n");
	printf("%0.5f,%0.5f,%0.5f\n",mag_cal_data.offset[VEC3_X],mag_cal_data.offset[VEC3_Y],
			mag_cal_data.offset[VEC3_Z]);*/
	use_mag_cal = 1;
}

int mpu9150_read_dmp(mpudata_t *mpu)
{
	short sensors;
	unsigned char more;

	if (data_ready() == 0) {
		return -1;
	}

	if (dmp_read_fifo(mpu->rawGyro, mpu->rawAccel, mpu->rawQuat, &mpu->dmpTimestamp, &sensors, &more) < 0) {
		fprintf(stderr, "[IMU_daemon] ERROR dmp_read_fifo() failed\n");
		return -1;
	}

	while (more) {
		// Fell behind, reading again
		if (dmp_read_fifo(mpu->rawGyro, mpu->rawAccel, mpu->rawQuat, &mpu->dmpTimestamp, &sensors, &more) < 0) {
			fprintf(stderr, "[IMU_daemon] ERROR dmp_read_fifo() failed\n");
			return -1;
		}
	}

	return 0;
}


int mpu9150_read_fifo(mpudata_t *mpu)
{
	unsigned char sensors;
	unsigned char more;

	if (data_ready() == 0) {
		return -1;
	}

	if (mpu_read_fifo(mpu->rawGyro, mpu->rawAccel, &mpu->dmpTimestamp, &sensors, &more) < 0) {
		fprintf(stderr, "[IMU_daemon] ERROR mpu_read_fifo() failed\n");
		return -1;
	}

	while (more) {
		// Fell behind, reading again
		if (mpu_read_fifo(mpu->rawGyro, mpu->rawAccel, &mpu->dmpTimestamp, &sensors, &more) < 0) {
			fprintf(stderr, "[IMU_daemon] ERROR mpu_read_fifo() failed\n");
			return -1;
		}
	}
}

int mpu9150_read_mag(mpudata_t *mpu)
{
	int status;
	if ((status = mpu_get_compass_reg(mpu->rawMag, &mpu->magTimestamp)) < 0) {
		fprintf(stderr, "[IMU_daemon] ERROR (%d) mpu_get_compass_reg() failed\n", status);
		return -1;
	}

	return 0;
}

int mpu9150_read(mpudata_t *mpu)
{
//	if (mpu9150_read_dmp(mpu) != 0)
	if (mpu9150_read_fifo(mpu) != 0)
		return -1;

	if (mpu9150_read_mag(mpu) != 0)
		return -1;

	calibrate_data(mpu);

	return data_fusion(mpu);
}

int data_ready()
{
	short status;

	if (mpu_get_int_status(&status) < 0) {
		fprintf(stderr, "[IMU_daemon] ERROR mpu_get_int_status() failed\n");
		return 0;
	}

	return (status == (MPU_INT_STATUS_DATA_READY /*| MPU_INT_STATUS_DMP | MPU_INT_STATUS_DMP_0*/));
}

void calibrate_data(mpudata_t *mpu)
{
	if (use_mag_cal) {
		double xc = mpu->rawMag[VEC3_X]-mag_cal_data.offset[VEC3_X];
		double yc = mpu->rawMag[VEC3_Y]-mag_cal_data.offset[VEC3_Y];
		double zc = mpu->rawMag[VEC3_Z]-mag_cal_data.offset[VEC3_Z];
		 	 	  		 
		mpu->calibratedMag[VEC3_X] = mag_cal_data.comp[0] * xc  +  mag_cal_data.comp[1] * yc  +  mag_cal_data.comp[2] * zc;
		mpu->calibratedMag[VEC3_Y] = mag_cal_data.comp[3] * xc  +  mag_cal_data.comp[4] * yc  +  mag_cal_data.comp[5] * zc;
		mpu->calibratedMag[VEC3_Z] = mag_cal_data.comp[6] * xc  +  mag_cal_data.comp[7] * yc  +  mag_cal_data.comp[8] * zc;

		mpu->M = sqrt(	mpu->calibratedMag[VEC3_X]*mpu->calibratedMag[VEC3_X] 
				+ 	mpu->calibratedMag[VEC3_Y]*mpu->calibratedMag[VEC3_Y] 
				+ 	mpu->calibratedMag[VEC3_Z]*mpu->calibratedMag[VEC3_Z] )*0.3;

		mpu->normMag[VEC3_Y] = mpu->calibratedMag[VEC3_X] / mpu->M;
		mpu->normMag[VEC3_X] = mpu->calibratedMag[VEC3_Y] / mpu->M;
		mpu->normMag[VEC3_Z] = mpu->calibratedMag[VEC3_Z] / mpu->M;

		mpu->absoluteMag[VEC3_X] = mpu->calibratedMag[VEC3_X]*MAG_SENSOR_SENSITIVITY;
		mpu->absoluteMag[VEC3_Y] = mpu->calibratedMag[VEC3_Y]*MAG_SENSOR_SENSITIVITY;
		mpu->absoluteMag[VEC3_Z] = mpu->calibratedMag[VEC3_Z]*MAG_SENSOR_SENSITIVITY;
		
	}
	else {
		mpu->calibratedMag[VEC3_X] = mpu->rawMag[VEC3_X];
		mpu->calibratedMag[VEC3_Y] = mpu->rawMag[VEC3_Y];
		mpu->calibratedMag[VEC3_Z] = mpu->rawMag[VEC3_Z];
	}

	if (use_accel_cal) {
		      mpu->calibratedAccel[VEC3_X] = ((double)mpu->rawAccel[VEC3_X] - accel_cal_data.offset[VEC3_X]) / accel_cal_data.range[VEC3_X];
		      mpu->calibratedAccel[VEC3_Y] = ((double)mpu->rawAccel[VEC3_Y] - accel_cal_data.offset[VEC3_Y])/accel_cal_data.range[VEC3_Y];
		      mpu->calibratedAccel[VEC3_Z] = ((double)mpu->rawAccel[VEC3_Z] - accel_cal_data.offset[VEC3_Z])/accel_cal_data.range[VEC3_Z];
	}
	else {
		mpu->calibratedAccel[VEC3_X] = mpu->rawAccel[VEC3_X];
		mpu->calibratedAccel[VEC3_Y] = mpu->rawAccel[VEC3_Y];
		mpu->calibratedAccel[VEC3_Z] = mpu->rawAccel[VEC3_Z];
	}
}

void tilt_compensate(quaternion_t magQ, quaternion_t unfusedQ)
{
	quaternion_t unfusedConjugateQ;
	quaternion_t tempQ;

	quaternionConjugate(unfusedQ, unfusedConjugateQ);
	quaternionMultiply(magQ, unfusedConjugateQ, tempQ);
	quaternionMultiply(unfusedQ, tempQ, magQ);
}

int data_fusion(mpudata_t *mpu)
{
	quaternion_t dmpQuat;
	vector3d_t dmpEuler;
	quaternion_t magQuat;
	quaternion_t unfusedQuat;
	float deltaDMPYaw;
	float deltaMagYaw;
	float newMagYaw;
	float newYaw;
	
	dmpQuat[QUAT_W] = (float)mpu->rawQuat[QUAT_W];
	dmpQuat[QUAT_X] = (float)mpu->rawQuat[QUAT_X];
	dmpQuat[QUAT_Y] = (float)mpu->rawQuat[QUAT_Y];
	dmpQuat[QUAT_Z] = (float)mpu->rawQuat[QUAT_Z];

	quaternionNormalize(dmpQuat);	
	quaternionToEuler(dmpQuat, dmpEuler);

	mpu->fusedEuler[VEC3_X] = dmpEuler[VEC3_X];
	mpu->fusedEuler[VEC3_Y] = -dmpEuler[VEC3_Y];
	mpu->fusedEuler[VEC3_Z] = 0;

	eulerToQuaternion(mpu->fusedEuler, unfusedQuat);

	deltaDMPYaw = -dmpEuler[VEC3_Z] + mpu->lastDMPYaw;
	mpu->lastDMPYaw = dmpEuler[VEC3_Z];

	magQuat[QUAT_W] = 0;
	magQuat[QUAT_X] = mpu->calibratedMag[VEC3_X];
  	magQuat[QUAT_Y] = mpu->calibratedMag[VEC3_Y];
  	magQuat[QUAT_Z] = mpu->calibratedMag[VEC3_Z];

	tilt_compensate(magQuat, unfusedQuat);

	newMagYaw = -atan2f(magQuat[QUAT_Y], magQuat[QUAT_X]);

	if (newMagYaw != newMagYaw) {
		printf("newMagYaw NAN\n");
		return -1;
	}

	if (newMagYaw < 0.0f)
		newMagYaw = TWO_PI + newMagYaw;

	newYaw = mpu->lastYaw + deltaDMPYaw;

	if (newYaw > TWO_PI)
		newYaw -= TWO_PI;
	else if (newYaw < 0.0f)
		newYaw += TWO_PI;
	 
	deltaMagYaw = newMagYaw - newYaw;
	
	if (deltaMagYaw >= (float)M_PI)
		deltaMagYaw -= TWO_PI;
	else if (deltaMagYaw < -(float)M_PI)
		deltaMagYaw += TWO_PI;

	if (yaw_mixing_factor > 0)
		newYaw += deltaMagYaw / yaw_mixing_factor;

	if (newYaw > TWO_PI)
		newYaw -= TWO_PI;
	else if (newYaw < 0.0f)
		newYaw += TWO_PI;

	mpu->lastYaw = newYaw;

	if (newYaw > (float)M_PI)
		newYaw -= TWO_PI;

	mpu->fusedEuler[VEC3_Z] = newYaw;

	eulerToQuaternion(mpu->fusedEuler, mpu->fusedQuat);

	return 0;
}

/* These next two functions convert the orientation matrix (see
 * gyro_orientation) to a scalar representation for use by the DMP.
 * NOTE: These functions are borrowed from InvenSense's MPL.
 */
unsigned short inv_row_2_scale(const signed char *row)
{
    unsigned short b;

    if (row[0] > 0)
        b = 0;
    else if (row[0] < 0)
        b = 4;
    else if (row[1] > 0)
        b = 1;
    else if (row[1] < 0)
        b = 5;
    else if (row[2] > 0)
        b = 2;
    else if (row[2] < 0)
        b = 6;
    else
        b = 7;      // error
    return b;
}

unsigned short inv_orientation_matrix_to_scalar(const signed char *mtx)
{
    unsigned short scalar;
    /*
       XYZ  010_001_000 Identity Matrix
       XZY  001_010_000
       YXZ  010_000_001
       YZX  000_010_001
       ZXY  001_000_010
       ZYX  000_001_010
     */
    scalar = inv_row_2_scale(mtx);
    scalar |= inv_row_2_scale(mtx + 3) << 3;
    scalar |= inv_row_2_scale(mtx + 6) << 6;
    return scalar;
}

