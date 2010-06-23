#ifndef __READIM7_H
#define __READIM7_H


#ifdef _WIN32

#	ifdef __BORLANDC__
#		pragma option -a1
#	else
#		pragma pack (1)
#	endif

#endif


enum ImageExtraFlag_t	// bit flags !
	{	IMAGE_EXTRA_FRAMETABLE	= 1,	// not used yet, TL 29.01.03
		IMAGE_EXTRA_2				= 2,
		IMAGE_EXTRA_3				= 4,
	};

typedef struct		// image file header (256 Bytes) for DaVis 7
{
	short int	version;			// 0: file version, increased by one at every header change
	short int	pack_type;		// 2: IM7PackType_t
	short int	buffer_format; // 4: BufferFormat_t
	short int	isSparse;		// 6: no (0), yes (1)
	int			sizeX;			// 8
	int 			sizeY;			// 12
	int 			sizeZ;			// 16
	int			sizeF;			// 20
	short int	scalarN;			// 24: number of scalar components
	short int	vector_grid; 	// 26: 1-n = vector data: grid size
	short int	extraFlags;		// 28: ImageExtraFlag_t
	// 30:
	char			reserved[256-30];
} Image_Header_7;


#ifdef _WIN32

#	ifdef __BORLANDC__
#		pragma option -a.
#	else
#		pragma pack ()
#	endif

#endif


// Returns error code ImReadError_t, can read IM7, VC7 and IMX, IMG, VEC
extern "C" int EXPORT ReadIM7 ( const char* theFileName, BufferType* myBuffer, AttributeList** myList );
extern "C" int EXPORT WriteIM7 ( const char* theFileName, bool isPackedIMX, BufferType* myBuffer );


#endif //__READIM7_H
