/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2022 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "uibmpreader.h"
/* local */
#include "uibitmap.h"
#include "uigraphics.h"
/* std */
#include <stdlib.h>
#include <stdio.h>


/* prototype */
static int psy_ui_bmpreader_read_header(psy_ui_BmpReader*, FILE*);
static int psy_ui_bmpreader_read_info_header(psy_ui_BmpReader*, FILE*);
static bool psy_ui_bmpreader_has_colour_map(const psy_ui_BmpReader*);
static int psy_ui_bmpreader_read_colour_map(psy_ui_BmpReader*, FILE*);
static int psy_ui_bmpreader_read_data_8_uncompressed(psy_ui_BmpReader*, FILE*,
	psy_ui_Graphics*);
static int psy_ui_bmpreader_read_data_8_rle(psy_ui_BmpReader*, FILE*,
	psy_ui_Graphics*);
static int psy_ui_bmpreader_read_rle(psy_ui_BmpReader*, uint8_t* v,
	uint8_t* escape, uint8_t* dx, uint8_t* dy, FILE*);
static int psy_ui_bmpreader_read_data_24(psy_ui_BmpReader*, FILE*,
	psy_ui_Graphics*);
static int psy_ui_bmpreader_read_padding(psy_ui_BmpReader*, FILE*);
static uint8_t psy_ui_bmpreader_padding(const psy_ui_BmpReader*);

static bool bigendian(void)
{
	union { int i; char c[sizeof(int)]; } u;
	u.i = 1;
	return !u.c[0];
}


void psy_ui_bmpreader_init(psy_ui_BmpReader* self,
	struct psy_ui_Bitmap* bitmap)
{
	self->bitmap = bitmap;
	self->verbose = 0;
}

void psy_ui_bmpreader_dispose(psy_ui_BmpReader* self)
{

}

int psy_ui_bmpreader_load(psy_ui_BmpReader* self, const char* path)
{
	FILE* fp;	
	int status;

	assert(self);
	
	if (!self->bitmap) {
		return PSY_ERRRUN;
	}	
	if (self->verbose) {
		printf("Image path = %s\n", path);
	}
	if ((fp = fopen(path, "rb")) == NULL) {
		return PSY_ERRFILEFORMAT;
	}
	psy_ui_bmpreader_read_header(self, fp);
	psy_ui_bmpreader_read_info_header(self, fp);
	if (psy_ui_bmpreader_has_colour_map(self)) {
		psy_ui_bmpreader_read_colour_map(self, fp);
	}
	/* seek to the start of the image data */
	fseek(fp, self->header.offset, SEEK_SET);
	if (self->verbose) {
		printf("Image size %d %d\n", self->infoheader.width,
			self->infoheader.height);
		printf("Image bits %d\n", (int)self->infoheader.bits);
		printf("Image compression %d\n", (int)self->infoheader.compression);
	}
	status = PSY_OK;
	if (self->infoheader.height > 0) {
		psy_ui_Graphics gc;
				
		psy_ui_bitmap_dispose(self->bitmap);
		psy_ui_bitmap_init_size(self->bitmap, psy_ui_realsize_make(
			self->infoheader.width, self->infoheader.height));
		psy_ui_graphics_init_bitmap(&gc, self->bitmap);		
		psy_ui_graphics_draw_solid_rectangle(&gc, psy_ui_realrectangle_make(
			psy_ui_realpoint_zero(), psy_ui_realsize_make(
				self->infoheader.width, self->infoheader.height)),
			psy_ui_colour_white());
		switch (self->infoheader.bits) {
		case 1:
			status = PSY_ERRFILEFORMAT;
			break;
		case 4:
			status = PSY_ERRFILEFORMAT;
			break;
		case 8:	
			if (self->infoheader.compression == 0) {
				status = psy_ui_bmpreader_read_data_8_uncompressed(self, fp,
					&gc);
			} else {
				status = psy_ui_bmpreader_read_data_8_rle(self, fp, &gc);
			}
			break;
		case 24:
			status = psy_ui_bmpreader_read_data_24(self, fp, &gc);
			break;
		default:
			status = PSY_ERRFILEFORMAT;
			break;
		}
		psy_ui_graphics_dispose(&gc);		
	}
	fclose(fp);
	return status;	
}

int psy_ui_bmpreader_read_header(psy_ui_BmpReader* self, FILE* fp)
{
	uintptr_t bytesRead;
	
	/* Magic identifier            */
	bytesRead = fread(&self->header.type, sizeof(char), 2, fp);
	if (self->header.type != 'M' * 256 + 'B') {
		/* todo warning */
	}
	/* File size in bytes          */
	bytesRead = fread(&self->header.size, sizeof(char), 4, fp);
	/* reserved					   */
	bytesRead = fread(&self->header.reserved1, sizeof(char), 2, fp);
	bytesRead = fread(&self->header.reserved2, sizeof(char), 2, fp);
	/* Offset to image data, bytes */
	bytesRead = fread(&self->header.offset, sizeof(char), 4, fp);
	return PSY_OK;
}

int psy_ui_bmpreader_read_info_header(psy_ui_BmpReader* self, FILE* fp)
{
	/* read and check the information header */
	if (fread(&self->infoheader, sizeof(psy_ui_BmpInfo), 1, fp) != 1) {
		return PSY_ERRFILE;
	}
	if (self->verbose) {		
		printf("Image size = %d x %d\n", self->infoheader.width,
			self->infoheader.height);
		printf("Number of colour planes is %d\n",
			self->infoheader.planes);
		printf("Bits per pixel is %d\n", self->infoheader.bits);
		printf("Compression type is %d\n",
			self->infoheader.compression);
		printf("Number of colours is %d\n", self->infoheader.ncolours);
		printf("Number of required colours is %d\n",
			self->infoheader.importantcolours);
	}
	return PSY_OK;
}

bool psy_ui_bmpreader_has_colour_map(const psy_ui_BmpReader* self)
{	
	return (self->infoheader.bits <= 8 && self->infoheader.ncolours > 0);
}

int psy_ui_bmpreader_read_colour_map(psy_ui_BmpReader* self, FILE* fp)
{
	uint32_t i;

	for (i = 0; i < 255; i++) {
		self->colourindex[i].r = 0;
		self->colourindex[i].g = 0;
		self->colourindex[i].b = 0;
		self->colourindex[i].junk = 0;	
	}	
	for (i = 0; i < self->infoheader.ncolours; i++) {
		if (fread(&self->colourindex[i].b, sizeof(unsigned char), 1, fp) != 1) {
			return PSY_ERRFILEFORMAT;
		}
		if (fread(&self->colourindex[i].g, sizeof(unsigned char), 1, fp) != 1) {			
			return PSY_ERRFILEFORMAT;
		}
		if (fread(&self->colourindex[i].r, sizeof(unsigned char), 1, fp) != 1) {
			return PSY_ERRFILEFORMAT;
		}
		if (fread(&self->colourindex[i].junk, sizeof(unsigned char), 1, fp) != 1) {
			return PSY_ERRFILEFORMAT;
		}
	}
	return PSY_OK;
}

int psy_ui_bmpreader_read_data_8_uncompressed(psy_ui_BmpReader* self, FILE* fp,
	psy_ui_Graphics* gc)
{	
	int32_t j;		
			
	for (j = self->infoheader.height - 1; j >= 0; j--) {
		int32_t i;		

		for (i = 0; i < self->infoheader.width; i++) {
			psy_ui_Colour colour;
			uint8_t r, g, b;
			uint8_t v;
				
			if (fread(&v, sizeof(unsigned char), 1, fp) != 1) {		
				return PSY_ERRFILEFORMAT;
			}			
			if (psy_ui_bmpreader_has_colour_map(self)) {
				r = self->colourindex[v].r;
				g = self->colourindex[v].g;
				b = self->colourindex[v].b;
			} else {
				r = (v >> 5) * 32;
				g = ((v & 28) >> 2) * 32;
				b = (v & 3) * 64;			
			}
			psy_ui_colour_init_rgb(&colour, r, g, b);
			psy_ui_graphics_draw_solid_rectangle(gc, psy_ui_realrectangle_make(
				psy_ui_realpoint_make(i, j), psy_ui_realsize_make(1.0, 1.0)),
				colour);
			if (self->verbose) {
				// fprintf(stderr, "Point %d, %d %x\n", i, j, (int)v);
			}			
		} /* i */
		psy_ui_bmpreader_read_padding(self, fp);
	} /* j */
	return PSY_OK;	
}

int psy_ui_bmpreader_read_data_8_rle(psy_ui_BmpReader* self, FILE* fp,
	psy_ui_Graphics* gc)
{	
	int32_t i;
	int32_t j;
	uint8_t escape;
			
	self->run_count = 0;	
	self->run_value = 0;	
	self->skip_count = 0;
	self->skip_padding = 2;		
	psy_ui_graphics_draw_solid_rectangle(gc, psy_ui_realrectangle_make(
		psy_ui_realpoint_zero(),
		psy_ui_realsize_make(self->infoheader.width, self->infoheader.height)),
		psy_ui_colour_black());
	i = 0;
	j = self->infoheader.height - 1;	
	while (j >= 0) {
		psy_ui_Colour colour;
		uint8_t r, g, b;
		uint8_t v;			
		uint8_t dx;
		uint8_t dy;
					
		escape = 0xFF;
		if (psy_ui_bmpreader_read_rle(self, &v, &escape, &dx, &dy, fp)) {					
			return PSY_ERRFILEFORMAT;
		}
		if (escape == psy_ui_BMPESCAPE_EOB) {			
			break;
		}				
		if (escape == psy_ui_BMPESCAPE_EOL) {
			if (j == 0) {
				break;
			}
			i = 0;			
			--j;			
			continue;
		}		
		if (escape == psy_ui_BMPESCAPE_DELTA) {
			i += dx;
			j += dy;
			continue;
		}
		if (psy_ui_bmpreader_has_colour_map(self)) {
			r = self->colourindex[v].r;
			g = self->colourindex[v].g;
			b = self->colourindex[v].b;
		} else {
			r = (v >> 5) * 32;
			g = ((v & 28) >> 2) * 32;
			b = (v & 3) * 64;			
		}
		psy_ui_colour_init_rgb(&colour, r, g, b);
		psy_ui_graphics_draw_solid_rectangle(gc, psy_ui_realrectangle_make(
			psy_ui_realpoint_make(i, j), psy_ui_realsize_make(1.0, 1.0)),
			colour);
		if (self->verbose) {
			// fprintf(stderr, "Point %d, %d %x\n", i, j, (int)v);
		}
		++i;		
	}
	return PSY_OK;	
}

int psy_ui_bmpreader_read_rle(psy_ui_BmpReader* self, uint8_t* v,
	uint8_t* escape, uint8_t* dx, uint8_t* dy, FILE* fp)
{	
	*escape = 0xFF;
	if (self->skip_count == 0 && self->run_count == 0) {
		if (fread(&self->run_count, sizeof(unsigned char), 1, fp) != 1) {
			return PSY_ERRFILEFORMAT;
		}		
		if (self->run_count == 0) {			
			if (fread(escape, sizeof(unsigned char), 1, fp) != 1) {
				return PSY_ERRFILEFORMAT;	
			}			
			if (*escape == 2) {
				if (fread(dx, sizeof(unsigned char), 1, fp) != 1) {
					return PSY_ERRFILEFORMAT;	
				}	
				if (fread(dy, sizeof(unsigned char), 1, fp) != 1) {
					return PSY_ERRFILEFORMAT;	
				}
			} else if (*escape > 2) {
				self->skip_count = *escape;
			} else {
				self->skip_count = 0;
			}
			self->skip_padding = 2;		
		} else {
			if (fread(&self->run_value, sizeof(unsigned char), 1, fp) != 1) {
				return PSY_ERRFILEFORMAT;
			}			
		}
	}	
	if (self->skip_count > 0) {
		if (fread(&self->run_value, sizeof(unsigned char), 1, fp) != 1) {
			return PSY_ERRFILEFORMAT;
		}		
		--self->skip_count;
		--self->skip_padding;
		if (self->skip_count == 0) {
			uint8_t i;
			uint8_t fill;	
			
			for (i = 0; i < self->skip_padding; ++i) {
				if (fread(&fill, sizeof(unsigned char), 1, fp) != 1) {
					return PSY_ERRFILEFORMAT;
				}
			}
			self->skip_padding = 2;
		} else {			
			if (self->skip_padding == 0) {
				self->skip_padding = 2;
			}
		}
	} else if (self->run_count > 0) {
		--self->run_count;
	}
	*v = self->run_value;
	return PSY_OK;
}

int psy_ui_bmpreader_read_data_24(psy_ui_BmpReader* self, FILE* fp,
	psy_ui_Graphics* gc)
{	
	int32_t j;		
		
	for (j = self->infoheader.height - 1; j >= 0; j--) {
		int32_t i;		

		for (i = 0; i < self->infoheader.width; i++) {
			psy_ui_Colour colour;
			uint8_t r, g, b;
				
			if (fread(&b, sizeof(unsigned char), 1, fp) != 1) {			
				return PSY_ERRFILEFORMAT;
			}
			if (fread(&g, sizeof(unsigned char), 1, fp) != 1) {
				return PSY_ERRFILEFORMAT;
			}
			if (fread(&r, sizeof(unsigned char), 1, fp) != 1) {
				return PSY_ERRFILEFORMAT;
			}					
			psy_ui_colour_init_rgb(&colour, r, g, b);
			psy_ui_graphics_draw_solid_rectangle(gc, psy_ui_realrectangle_make(
				psy_ui_realpoint_make(i, j), psy_ui_realsize_make(1.0, 1.0)),
				colour);
			if (self->verbose) {
				printf("Point = %d , %d, %d, %d, %d\n", i, j,
					(int)r, (int)g, (int)b);
			}			
		} /* i */
		psy_ui_bmpreader_read_padding(self, fp);
	} /* j */
	return PSY_OK;	
}

int psy_ui_bmpreader_read_padding(psy_ui_BmpReader* self, FILE* fp)
{
	uint8_t padding;	
	uint8_t i;
	
	padding = psy_ui_bmpreader_padding(self);
	for (i = 0; i < padding; ++i) {
		uint8_t v;
		
		fread(&v, sizeof(unsigned char), 1, fp);
	}
	return PSY_OK;	
}

/* 
** Each scan line is zero padded to the nearest 4-byte boundary. If the image
** has a width that is not divisible by four, say, 21 bytes, there would be
** 3 bytes of padding at the end of every scan line.
*/
uint8_t psy_ui_bmpreader_padding(const psy_ui_BmpReader* self)
{
	uint8_t num_bytes;
	
	switch (self->infoheader.bits) {
	case 16:
		num_bytes = 2;
		break;
	case 24:
		num_bytes = 3;
		break;		
	default:
		num_bytes = 1;
		break;
	}
	return ((uint8_t)(ceil((self->infoheader.width * num_bytes) / 4.0)) * 4 -
		(self->infoheader.width * num_bytes));				
}
