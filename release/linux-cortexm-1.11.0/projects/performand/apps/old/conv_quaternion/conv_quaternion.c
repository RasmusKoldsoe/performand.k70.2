#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/time.h>
#include <math.h>
#include <signal.h>
#include <getopt.h>
#include <unistd.h>
#include <math.h>

void register_sig_handler();
void sigint_handler(int sig);

int done;

#define	DEGREE_TO_RAD		((float)M_PI / 180.0f)
#define	RAD_TO_DEGREE		(180.0f / (float)M_PI)

#define TWO_PI				(2.0f * (float)M_PI)

#define QUAT_W		0
#define QUAT_X		1
#define QUAT_Y		2
#define QUAT_Z		3
typedef float quaternion_t[4]; 

#define VEC3_X		0
#define VEC3_Y		1
#define VEC3_Z		2
typedef float vector3d_t[3];


void quaternionToEuler(quaternion_t q, vector3d_t v)
{
	// fix roll near poles with this tolerance
	float pole = (float)M_PI / 2.0f - 0.05f;

	v[VEC3_Y] = asinf(2.0f * (q[QUAT_W] * q[QUAT_Y] - q[QUAT_X] * q[QUAT_Z]));

	if ((v[VEC3_Y] < pole) && (v[VEC3_Y] > -pole)) {
		v[VEC3_X] = atan2f(2.0f * (q[QUAT_Y] * q[QUAT_Z] + q[QUAT_W] * q[QUAT_X]),
					1.0f - 2.0f * (q[QUAT_X] * q[QUAT_X] + q[QUAT_Y] * q[QUAT_Y]));
	}

	v[VEC3_Z] = atan2f(2.0f * (q[QUAT_X] * q[QUAT_Y] + q[QUAT_W] * q[QUAT_Z]),
					1.0f - 2.0f * (q[QUAT_Y] * q[QUAT_Y] + q[QUAT_Z] * q[QUAT_Z]));
}

int main(void) {
	FILE * fp;
	char * line = NULL;
	size_t len = 0;
	ssize_t read;
	int line_counter=0;
  	unsigned long qw,qx,qy,qz;
	quaternion_t q_t;
	vector3d_t v_t;

	register_sig_handler();

	while(!done) {
		fp = fopen("/dev/imu", "r");
		if (fp == NULL)
	  		exit(-1);
		line_counter=0;
		while ((read = getline(&line, &len, fp)) != -1) {		   
		   switch (line_counter++) {
			case 1: 
				sscanf(line,"%*[^0123456789]%lu",&qw);				
				//printf("Q_W: %lu\n",qw); 
				break;
			case 2: 
				sscanf(line,"%*[^0123456789]%lu",&qx);				
				//printf("Q_X: %lu\n",qx); ; 
				break;
			case 3: 
				sscanf(line,"%*[^0123456789]%lu",&qy);				
				//printf("Q_Y: %lu\n",qy);
				break;
			case 4: 
				sscanf(line,"%*[^0123456789]%lu",&qz);				
				//printf("Q_Z: %lu\n",qz);
				break;
			case 6: 
				//printf("Time\n"); 
				break;
			case 9: 
				//printf("Mag_X\n"); 
				break;
			case 10: 
				//printf("Mag_Y\n"); 
				break;
			case 11: 
				//printf("Mag_Z\n"); 
				break;
			case 12: 
				//printf("Mag Time\n"); 
				break;
			default: 
				//printf("%d ist irgendwas\n",line_counter); 
				break;
		   }
		}
	
		if (line)
		   free(line);

		fclose(fp);
		//printf("\rW: %lu X: %lu Y: %lu Z: %lu       ",qw,qx,qy,qz);
		//fflush(stdout);
		
		q_t[QUAT_W] = qw;
		q_t[QUAT_X] = qx;
		q_t[QUAT_Y] = qy;
		q_t[QUAT_Z] = qz;

		quaternionToEuler(q_t, v_t);

		printf("\rX: %f Y: %f Z: %f",v_t[VEC3_X]
						,v_t[VEC3_Y]
						,v_t[VEC3_Z]);
		fflush(stdout);

		usleep(100000);
	}

	exit(1);
}

void register_sig_handler()
{
	struct sigaction sia;

	bzero(&sia, sizeof sia);
	sia.sa_handler = sigint_handler;

	if (sigaction(SIGINT, &sia, NULL) < 0) {
		perror("sigaction(SIGINT)");
		exit(1);
	} 
}

void sigint_handler(int sig)
{
	done = 1;
}
