
#include <iostream>
#include <string>

typedef unsigned int quint32;
typedef unsigned short quint16;
typedef unsigned char quint8;
typedef std::string qstring;

#define BMP_MAGIC_NUMBER	19778
#define RGB_BYTE_SIZE		3
#define FILE_HEADER_SIZE	14
#define INFO_HEADER_SIZE	40

struct bmpfileheader
{
	quint16	id;				// 0
	quint32	filesize;		// 2
	quint16	reserved1;		// 6
	quint16	reserved2;		// 8
	quint32	imageoffset;	// 10
};

struct bmpinfoheader
{
	quint32	headersize;		// 14
	int		width;			// 18
	int		height;			// 22
	quint16	planes;			// 26
	quint16	bitcount;		// 28
	quint32	compression;	// 30
	quint32	imagesize;		// 34
	int		pixelpermeterx;	// 38
	int		pixelpermetery;
	quint32	clrused;
	quint32	clrimportant;
};

size_t FromBMP(quint8** out, int& outwidth, int& outheight, int& outbytes, size_t size, const void* data)
{
	bmpinfoheader	info;
	bmpfileheader	header;
	char*			fh = NULL;
	char*			ih = NULL;
	char*			imgdata = (char*)data;
	quint8*			outdata = NULL;
	int*			palette = NULL;
	size_t			ret = 0;
	int				palettesize;
	int				bytes;
	int				linesize;
	int				padding;

	fh = (char*)malloc(FILE_HEADER_SIZE);
	ih = (char*)malloc(INFO_HEADER_SIZE);

	memcpy(fh, imgdata, FILE_HEADER_SIZE);
	imgdata += FILE_HEADER_SIZE;

	if( *((quint16*)fh) != BMP_MAGIC_NUMBER )
		goto _cleanup;

	memcpy(ih, imgdata, INFO_HEADER_SIZE);
	imgdata += INFO_HEADER_SIZE;

	header.filesize		= *((quint32*)(fh + 2));
	header.imageoffset = *((quint32*)(fh + 10));

	info.width			= *((int*)(ih + 4));
	info.height			= *((int*)(ih + 8));
	info.bitcount		= *((quint16*)(ih + 14));
	info.compression	= *((quint32*)(ih + 16));
	info.pixelpermeterx	= *((int*)(ih + 24));
	info.pixelpermetery	= *((int*)(ih + 28));

	bytes = info.bitcount / 8;
	info.imagesize = info.width * info.height * bytes;

	linesize = info.width * bytes;
	padding = (4 - (linesize % 4)) % 4;

	free(fh);
	free(ih);

	fh = 0;
	ih = 0;

	if( info.bitcount < 8 )
		goto _cleanup;

	if( info.compression > 2 )
		goto _cleanup;

	palettesize = (header.imageoffset - (FILE_HEADER_SIZE + INFO_HEADER_SIZE)) / 4;
	palette = NULL;

	if( palettesize > 0 )
	{
		palette = (int*)malloc(palettesize * sizeof(int));

		memcpy(palette, imgdata, palettesize * sizeof(int));
		imgdata += palettesize * sizeof(int);
	}

	if( info.imagesize == 0 )
		info.imagesize = size - header.imageoffset;

	imgdata = ((char*)data + header.imageoffset);
	outdata = new quint8[info.imagesize];

	if( padding == 0 )
		memcpy(outdata, imgdata, info.imagesize);
	else
	{
		for( int i = 0; i < info.height; ++i )
		{
			memcpy(
				outdata + i * info.width * bytes,
				imgdata + i * ((info.width * bytes) + padding),
				linesize - padding);
		}
	}

	ret = size;
	(*out) = outdata;

	outwidth = info.width;
	outheight = info.height;
	outbytes = bytes;

_cleanup:
	free(fh);
	free(ih);
	free(palette);

	return ret;
}

bool FromBMP(quint8** out, int& outwidth, int& outheight, int& outbytes, const qstring& file)
{
	FILE* infile = fopen(file.c_str(), "rb");;

	if( !infile )
		return false;

	fseek(infile, 0, SEEK_END);
	long length = ftell(infile);
	fseek(infile, 0, SEEK_SET);

	char* data = (char*)malloc(length);

	if( !data )
	{
		fclose(infile);
		return false;
	}

	fread(data, 1, length, infile);
	fclose(infile);

	size_t size = FromBMP(out, outwidth, outheight, outbytes, (size_t)length, data);
	free(data);

	return (0 != size);
}

bool ToBMP(quint8* data, int width, int height, int bytes, const qstring& file)
{
	FILE* outfile = fopen(file.c_str(), "wb");;

	if( !outfile )
		return false;

	bmpinfoheader info;
	bmpfileheader header;
	int paddedwidth = ((width * 3 + 3) / 4) * 4;

	info.width			= width;
	info.height			= height;
	info.bitcount		= 24;
	info.compression	= 0;
	info.imagesize		= width * height * 3;
	info.headersize		= INFO_HEADER_SIZE;

	header.imageoffset = FILE_HEADER_SIZE + INFO_HEADER_SIZE;
	header.filesize = header.imageoffset + (paddedwidth * height);

	// write headers
	char* fh = new char[FILE_HEADER_SIZE];
	char* ih = new char[INFO_HEADER_SIZE];

	memset(fh, 0, FILE_HEADER_SIZE);
	memset(ih, 0, INFO_HEADER_SIZE);

	*((quint16*)fh)			= BMP_MAGIC_NUMBER;
	*((quint32*)(fh + 2))	= header.filesize;
	*((quint32*)(fh + 10))	= header.imageoffset;

	*((quint32*)(ih))		= info.headersize;
	*((int*)(ih + 4))		= info.width;
	*((int*)(ih + 8))		= info.height;
	*((quint16*)(ih + 12))	= 1;
	*((quint16*)(ih + 14))	= info.bitcount;
	*((quint32*)(ih + 16))	= info.compression;
	*((quint32*)(ih + 20))	= 0;

	fwrite(fh, FILE_HEADER_SIZE, 1, outfile);
	fwrite(ih, INFO_HEADER_SIZE, 1, outfile);

	delete[] fh;
	delete[] ih;

	if( bytes == 4 )
	{
		quint8* newdata = new quint8[width * height * 3];

		for( int i = 0; i < height; ++i )
		{
			for( int j = 0; j < width; ++j )
			{
				newdata[(i * width + j) * 3 + 0] = data[(i * width + j) * 4 + 0];
				newdata[(i * width + j) * 3 + 1] = data[(i * width + j) * 4 + 1];
				newdata[(i * width + j) * 3 + 2] = data[(i * width + j) * 4 + 2];
			}
		}

		fwrite(newdata, 1, info.imagesize, outfile);
		paddedwidth = paddedwidth * height - info.imagesize;

		memset(newdata, 0, paddedwidth);
		fwrite(newdata, 1, paddedwidth, outfile);

		delete[] newdata;
	}
	else if( bytes == 3 )
	{
		int linesize = width * bytes;
		int padding = (4 - (linesize % 4)) % 4;

		if( padding == 0 )
			fwrite(data, 1, info.imagesize, outfile);
		else
		{
			for( int i = 0; i < height; ++i )
			{
				fwrite(data + i * width * bytes, 1, width * bytes, outfile);
				fwrite("\0\0\0\0", 1, padding, outfile);
			}
		}
	}

	fclose(outfile);
	return true;
}

quint8* ConvertTo32BPP(quint8* data24bpp, int width, int height)
{
	quint8* outdata = new quint8[width *  height * 4];
	quint8 r, g, b;
	size_t index;

	for( int i = 0; i < height; ++i )
	{
		for( int j = 0; j < width; ++j )
		{
			index = (i * width + j) * 3;

			r = data24bpp[index + 0];
			g = data24bpp[index + 1];
			b = data24bpp[index + 2];

			index = (i * width + j) * 4;

			outdata[index + 0] = r;
			outdata[index + 1] = g;
			outdata[index + 2] = b;
			outdata[index + 3] = 255;
		}
	}

	delete[] data24bpp;
	return outdata;
}

bool ToTGA(quint8* data, int width, int height, int bytes, const qstring& file)
{
	FILE* outfile = fopen(file.c_str(), "wb");
	
	if (!outfile)
		return false;

	quint8 header[18];
	memset(header, 0, 18);

	header[2] = 2; // type

	*((quint16*)(&header[12])) = (quint16)width;
	*((quint16*)(&header[14])) = (quint16)height;

	header[16] = bytes * 8;
	fwrite(header, 1, 18, outfile);

	if( bytes == 3 )
		fwrite(data, 1, width * height * 3, outfile);
	else
		fwrite(data, 1, width * height * 4, outfile);

	fclose(outfile);
	return true;
}

int main ()
{
	quint8* img1 = 0;
	quint8* img2 = 0;
	quint8* img3;
	int width, height, bytes;

	if( !FromBMP(&img1, width, height, bytes, "../resources/1.bmp") )
		return 1;

	if( !FromBMP(&img2, width, height, bytes, "../resources/2.bmp") )
		return 1;

	img1 = ConvertTo32BPP(img1, width, height);
	img2 = ConvertTo32BPP(img2, width, height);
	img3 = new quint8[width * height * 4];

	quint8 r, g, b;
	double a, x, y, z;
	quint8 r1, g1, b1;
	quint8 r2, g2, b2;
	quint8 c1, c2;
	int index;

	for( int i = 0; i < height; ++i )
	{
		for( int j = 0; j < width; ++j )
		{
			index = (i * width + j) * 4;

			//if( i == height - 126 - 1 && j == 122 )
			//	::_CrtDbgBreak();

			r1 = img1[index + 0];
			g1 = img1[index + 1];
			b1 = img1[index + 2];

			r2 = img2[index + 0];
			g2 = img2[index + 1];
			b2 = img2[index + 2];

			if( r1 == r2 && g1 == g2 && b1 == b2 )
			{
				// same -> no alpha
				a = 1.0;
				r = r1;
				g = g1;
				b = b1;
			}
			else
			{
				c1 = 255;
				c2 = 0;

				if( r1 - r2 < 255 )
				{
					c1 = c1;
					c2 = r2;
				}

				if( g1 - g2 < 255 )
				{
					if( g2 > c2 )
					{
						c1 = c1;
						c2 = g2;
					}
				}

				if( b1 - b2 < 255 )
				{
					if( b2 > c2 )
					{
						c1 = c1;
						c2 = b2;
					}
				}

				if( c1 - c2 < 255 )
				{
					a = (255.0 - (c1 - c2)) / 255.0;

					//if( a > 1.0 || a < 0.0 )
					//	::_CrtDbgBreak();

					x = std::min<double>(r2 / a, 255.0);
					y = std::min<double>(g2 / a, 255.0);
					z = std::min<double>(b2 / a, 255.0);

					r = (quint8)x;
					g = (quint8)y;
					b = (quint8)z;
				}
				else
				{
					// background in both
					a = 0.0;
					r = 0;
					g = 0;
					b = 0;
				}
			}

			img3[index + 0] = r;
			img3[index + 1] = g;
			img3[index + 2] = b;
			img3[index + 3] = (quint8)(a * 255.0);
		}
	}

	ToTGA(img3, width, height, 4, "../resources/test.tga");

	delete[] img1;
	delete[] img2;
	delete[] img3;

	//system("pause");
	return 0;
}
