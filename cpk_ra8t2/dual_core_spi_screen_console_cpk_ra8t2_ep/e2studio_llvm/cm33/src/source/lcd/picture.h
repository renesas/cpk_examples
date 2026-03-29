#ifndef __PICTURE_H
#define __PICTURE_H

struct gimp_image {
	char *comment;
	unsigned int width;
	unsigned int height;
	unsigned int bytes_per_pixel; /* 2:RGB16, 3:RGB, 4:RGBA */
	const unsigned char *pixel_data;
};

extern const struct gimp_image g_picture01;
extern const struct gimp_image g_picture02;

#endif
