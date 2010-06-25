/*
	ReadIMX.dll
	(c) LaVision
	TL 10.05.01: reading IMG/IMX/VEC raw data
	TL 11.06.01: added reading of string attributes
	TL 30.09.02: corrected mis-aligned image_header for Linux
	TL 05.02.04: support IM7/VC7 on WIN32
	TL 22.03.04: BufferScaleType and reading of scaling information
	TL 09.07.04: DestroyBuffer
	TL 08.12.04: LabView functions
	TL 13.12.04: WriteIMG
	TL 18.02.05: DestroyAttributeList
	TL 20.09.05: WriteIMX
	TL 07.03.06: LabView functions to write number of frame
	TL 02.11.06: Labiew functions remember the attribute list, WriteImgAttributes
	TL 16.02.09: ReadIM7 with 12 bit compression

	Read a LaVision IMX/IMG/VEC file by a call to:

	extern "C" int EXPORT ReadIMX ( const char* theFileName, BufferType* myBuffer, AttributeList** myList )

	If you don't want to read the attributes, set myList = NULL. 
	If myList is set, the parameter returns a pointer to the list. 
	You have to free the strings manually or call DestroyAttributeList.

	The function returns the error codes of ImReadError_t.
*/

#include "ReadIMX.h"

#ifdef _LINUX
#	define max(v1,v2)	(v1 > v2 ? v1 : v2)
#else // Windows
	// TL 21.04.09: strcpy_s Warnungen weg
#	pragma warning (disable : 4996)
#endif


/*****************************************************************************/
// General functions and header (image_header) of IMG/IMX/VEC files
/*****************************************************************************/

#ifdef _WIN32

#	ifdef __BORLANDC__
#		pragma option -a1
#	else
#		pragma pack (1)
#	endif

#endif


typedef struct		// image file header (256 Bytes)
	{
	 short int 	imagetype;      // 0:  (Image_t)
	 short int	xstart;			// 2:  start-pos left, not used
	 short int	ystart;			// 4:  start-pos top, not used
	 Byte			extended[4];// 6:  reserved
	 short int	rows;			// 10: total number of rows, if -1 the real number is stored in longRows
	 short int	columns;		// 12: total number of columns, if -1 see longColumns
	 short int  image_sub_type; // 14: type of image (int):
								//				(included starting from sc_version 4.2 on)
								// 	0 = normal image
								//		1 = PIV vector field with header and 4*2D field
								//				rows = 9 * y-size
								//		2 = simple 2D vector field (not used yet)
								//				rows = 2 * y-size
								//		3 ...
	 short int 	y_dim;			// 16: size of y-dimension (size of x-dimension is always = columns), not used
	 short int 	f_dim;			// 18: size of f-dimension (number of frames)

	 // for image_sub_type 1/2 only:
	 short int	vector_grid; 	// 20: 1-n = vector data: grid size
								//     		(included starting from sc_version 4.2 on)
	 char			ext[11];	// 22: reserved
	 Byte			version;	// 33:  e.g. 120 = 1.2	300+ = 3.xx  40-99 = 4.0 bis 9.9
#ifdef _WIN32
	 char       date[9];        // 34	
	 char       time[9];		// 43
#else
//#message MIS-ALIGNED!!!
	 char       date[8];        // 34	
	 char       time[8];		// 43
#endif
	 short int  xinit;          // 52:  x-scale values
    float       xa;             // 54
    float       xb;             // 58
    char        xdim[11];       // 62
    char        xunits[11];     // 73
	 short int  yinit;          // 84:  y-scale values
    float       ya;             // 86
    float       yb;             // 90
    char        ydim[11];       // 94
    char        yunits[11];     // 105
	 short int  iinit;          // 116: i-scale (intensity) values
    float       ia;             // 118
    float       ib;             // 122
    char        idim[11];       // 126
    char        iunits[11];     // 137

	 char       com1[40];       // 148: comment (2 lines)
	 char       com2[40];		// 188

	 int		longRows;	    // 228: (large) number of rows, TL 04.02.2000
	 int		longColumns;    // 232: (large) number of columns, TL 04.02.2000
     int		longZDim;		// 236: (large) number of z-dimension, TL 02.05.00
	 char		reserved[12];   // 240: reserved
	 int		checksum;	    // 252-255: not used
	} image_header;


#ifdef __BORLANDC__
#	pragma option -a.
#else
#	pragma pack ()
#endif


#define VER_EXTHEADER			54
#define VER_VOLUME_BUFFER		55


extern "C" void EXPORT SetBufferScale( BufferScaleType* theScale, float theFactor, float theOffset, const char* theDesc, const char* theUnit )
{
	if (theScale)
	{
		theScale->factor	= theFactor;
		theScale->offset	= theOffset;
		strncpy( theScale->description, theDesc, sizeof(theScale->description) );
		theScale->description[sizeof(theScale->description)-1] = 0;
		strncpy( theScale->unit, theUnit, sizeof(theScale->unit) );
		theScale->unit[sizeof(theScale->unit)-1] = 0;
	}
}


int CreateBuffer( BufferType* myBuffer, int theNX, int theNY, int theNZ, int theNF, int isFloat, int vectorGrid, BufferFormat_t imageSubType )
{
	if (myBuffer==NULL)
		return 0;
   myBuffer->isFloat = isFloat;
   myBuffer->nx = theNX;
   myBuffer->ny = theNY;
   myBuffer->nz = theNZ;
   myBuffer->nf = theNF;
   myBuffer->totalLines = theNY*theNZ*theNF;
	myBuffer->vectorGrid = vectorGrid;
	myBuffer->image_sub_type = imageSubType;
   int size = theNX * myBuffer->totalLines;
   if (isFloat)
	{
      myBuffer->floatArray = (float*)malloc(sizeof(float)*size);
   }
	else
	{
      myBuffer->wordArray = (Word*)malloc(sizeof(Word)*size);
   }
	SetBufferScale( &myBuffer->scaleX, 1, 0, "", "pixel" );
	SetBufferScale( &myBuffer->scaleY, 1, 0, "", "pixel" );
	SetBufferScale( &myBuffer->scaleI, 1, 0, "", "counts" );
   return (myBuffer->floatArray!=NULL || myBuffer->wordArray!=NULL);
}


void DestroyBuffer( BufferType* myBuffer )
{
	if (myBuffer)
	{
		if (myBuffer->isFloat)
		{
			free(myBuffer->floatArray);
			myBuffer->floatArray = NULL;
		}
		else
		{
			free(myBuffer->wordArray);
			myBuffer->wordArray = NULL;
		}
		myBuffer->nx = 0;
		myBuffer->ny = 0;
		myBuffer->nz = 0;
		myBuffer->nf = 0;
	}
}


void DestroyAttributeList( AttributeList** myList )
{
	AttributeList* item;
	while (*myList)
	{
		item = *myList;
		*myList = (*myList)->next;
		free(item->name);
		free(item->value);
		free(item);
	}
}


Byte* Buffer_GetRowAddrAndSize( BufferType* myBuffer, int theRow, unsigned long &theRowLength )
{
	theRowLength = myBuffer->nx * (myBuffer->isFloat?sizeof(float):sizeof(Word));
	if (myBuffer->isFloat)
		return (Byte*)&myBuffer->floatArray[myBuffer->nx*theRow];
	return    (Byte*)&myBuffer->wordArray [myBuffer->nx*theRow];
}



/*****************************************************************************/
// Reading IMG and IMX
/*****************************************************************************/


typedef enum 
{
	BIT8,		// compressed as 8 bit
	BIT4L,		// compressed to 4 bit left nibble
	BIT4R,		// compressed to 4 bit right nibble
	BIT16		// n-uncompressed 2-byte pixels
} CompressionMode_t;		// compression mode
/* compression syntax:
		BIT8: - At start lastvalue = 0 and cmode = BIT8, i.e. it is assumed
		        that the following bytes are coded with 1 pixel per byte
		BIT8: - If -128 is encountered, a pixel as a Word (2 bytes) follows.
		        This value is the absolute value of the pixel.
		BIT8: - If -127 is encountered cmode is set to BIT4, i.e. from now
		        on, each byte contains 2 pixels, each consisting of 4 bits.
		        Thus each new value can take on values between -7 and +7.
		        This is coded as 0x00-0x07 (0-7) and 0x09-0x0F (-7 to -1)
		BIT8: - If 127 is encountered, then the next byte gives the number
		        of uncompressed 16-bit pixel values, followed by the 2-byte data.
		BIT4: - Cmode BIT4 is set back to BIT8 if 0x08 is encountered in the
		        right 4 bits.
*/


const size_t PAGESIZE = 0x800;		// Words !  size must not be too big!

long IMX_pagepos = 0;   // TL 16.12.1999

// read data from the image file to the buffer, reset 2 variables
// called by SCBuffer:Read
Word ReadNextPage (FILE* file, Word* page, signed char** bpageadr, Word* count)
{
	IMX_pagepos = ftell(file);
	if (fread(page, 1, PAGESIZE, file)==0)
		return 0;			// read error? (not TRUE if EOF)
	else
	{	*count = 0;
		*bpageadr = (signed char*) page;
		return (Word)(ftell(file) - IMX_pagepos);
	}
}


ImReadError_t SCPackOldIMX_Read( FILE* theFile, BufferType* myBuffer )
{
	// Yes. Skip the preview image stored after the header
	Word bytecount, count, lastvalue, newvalue = 0;
	signed char *bpageadr, bvalue = 0, newnibble;
	Byte nx, ny, nbytes = 0;
	int bline;
	Word  poge [PAGESIZE];							// input buffer to increase performance
	Word* page = (Word*)&poge;						// by reducing the # of read operations
	CompressionMode_t cmode = BIT8;                               // start with BIT8 mode

	if (!fread(&nx,1,1,theFile)) 	goto errexit;	   // read x-size of preview
	if (!fread(&ny,1,1,theFile))  goto errexit;		// read y-size of preview
	// Preview: The preview includes the upper 8 bit of a restricted number of pixel.
	//		int steps = max( myBuffer->nx / 100 + 1, myBuffer->ny / 100 + 1 );
	//		Byte ny = (myBuffer->ny-1) / steps + 1;
	//		Byte nx = (myBuffer->nx-1) / steps + 1;
	//		Byte *myPreview = malloc(sizeof(Byte)*ny*nx);
	//		for (int y=0; y<ny; y++)
	//			for (int x=0; x<nx; x++)
	//				fread(&myPreview[nx*y+x],1,1,theFile);
	if (fseek( theFile, nx*ny, SEEK_CUR))  goto errexit;	// skip preview

	lastvalue = 0;                              // and assuming a previous 0
	if ( (bytecount = ReadNextPage(theFile, page, &bpageadr, &count)) == 0 )  goto errexit;

	for (bline = 0; bline < myBuffer->totalLines; bline++ )
	{
		Word* rowW = &myBuffer->wordArray[myBuffer->nx * bline];
		for (int i = 0; i < myBuffer->nx; i++ )
		{
			if ( count == bytecount )               // no more bytes left?
				if ( (bytecount = ReadNextPage(theFile, page, &bpageadr, &count)) == 0 )
					goto errexit;	// error 5
			if ( cmode == BIT4L || cmode == BIT8 )  // need new byte
			{
				bvalue = *bpageadr++;                 // new value
				count++;                              // 1 byte processed
			}

			if ( cmode == BIT4R )                   // process right nibble next
			{
				newnibble = bvalue & 0x0F;            // get right nibble
				cmode = BIT4L;                        // next: left nibble
				if ( newnibble == 0x08 )              // change back to BIT8
				{
					cmode = BIT8;
					if ( count == bytecount )           // no more bytes left?
						if ( (bytecount = ReadNextPage(theFile, page, &bpageadr, &count)) == 0 )
							goto errexit;
					bvalue = *bpageadr++;               // new value BIT8-mode
					count++;                            // 1 byte processed
				}
				else
				{
					if ( newnibble & 0x08 ) newnibble |= 0xF0;  // -1 to -7
					newvalue = lastvalue + (long)newnibble;
				}
			}
			else if ( cmode == BIT4L )              // process left nibble next
			{
				newnibble = bvalue >> 4;
				if ( newnibble & 0x08 ) newnibble |= 0xF0;    // -1 to -7
				newvalue = lastvalue + (long)newnibble;       // get left nibble
				cmode = BIT4R;                        // next: right nibble
			}

			if ( cmode == BIT8 )                    // BIT8 compression mode
			{
				switch ( bvalue )
				{
				case -128:                     // exception?
					cmode = BIT16;              // change to BIT16
					nbytes = 1;                 // read only 1 pixel
					break;
				case 127:                      // exception? n-16-bit values; number of bytes follows
					if ( count == bytecount )         // no more bytes left?
						if ( (bytecount = ReadNextPage(theFile, page, &bpageadr, &count)) == 0 )
							goto errexit;
					cmode = BIT16;                    // set cmode correctly
					nbytes = *( (Byte*)bpageadr++ );  // number of bytes
					count++;                          // 1 byte read
					break;
				case -127:                          // change to BIT4 mode
					if ( count == bytecount )         // no more bytes left?
						if ( (bytecount = ReadNextPage(theFile, page, &bpageadr, &count)) == 0 )
							goto errexit;
					bvalue = *bpageadr++;             // get new byte
					count++;
					newnibble = bvalue >> 4;
					if ( newnibble & 0x08 ) newnibble |= 0xF0; // -1 to -7
					newvalue = lastvalue + (long)newnibble;    // get left nibble
					cmode = BIT4R;                    // process right nibble next
					break;
				default:
					newvalue = lastvalue + (long)bvalue;  // get new value, normal mode
				}
			}			// if cmode = BIT8

			if ( cmode == BIT16 )                   // nbytes 16-bit pixels
			{
				if ( count == bytecount )             // no more bytes left?
					if ( (bytecount = ReadNextPage(theFile, page, &bpageadr,	&count)) == 0 )
						goto errexit;		// error 5
				if ( count == bytecount-1 )           // only one byte left?
				{                                     // ---> trouble!
					newvalue = (Word)*(Byte*)bpageadr;  // get low byte first from old page
					if ( (bytecount = ReadNextPage(theFile, page, &bpageadr,	&count)) == 0 )
						goto errexit;		// error 5
					newvalue += ((Word)*(Byte*)bpageadr++) << 8; // get high byte from new page
					count = 1;                          // one byte used already
				}
				else
				{
					newvalue = *( (Word*)bpageadr );    // get new value, Word pointer!
					bpageadr += 2;                      // increment byte pointer
					count += 2;                         // 2 bytes processed
				}
				if ( --nbytes == 0 ) cmode = BIT8;    // decrement counter, change mode
			}

			if ( i < myBuffer->nx )                     // maybe skip right part
				rowW[i] = newvalue;
				//pixbuf.W[bline][i] = newvalue;			// store in pixbuf
			lastvalue = newvalue;                  // update last value

		} 	// for i
	}	// for bline

	fseek( theFile, IMX_pagepos + ((char*)bpageadr - (char*)page), SEEK_SET );
	return IMREAD_ERR_NO;

errexit:
	return IMREAD_ERR_DATA;
}


extern "C" int EXPORT ReadIMX ( const char* theFileName, BufferType* myBuffer, AttributeList** myList )
{
	image_header header;

	FILE* theFile = fopen(theFileName, "rb");				// open for binary read
	if (theFile==NULL)
	   return IMREAD_ERR_FILEOPEN;

	// Read an image in our own IMX or IMG or VEC or VOL format
	int theNX,theNY,theNZ,theNF;
	// read and store file header contents
	if (!fread ((char*)&header, sizeof(header), 1, theFile))
	{
      fclose(theFile);
      return IMREAD_ERR_HEADER;
   }
   
	int itsVersion	= header.version;
	//itsDate		= header.date;
	//itsTime		= header.time;

   // scales
   char olddim[12], oldunit[12];
   strncpy(olddim,header.xdim,11); olddim[11]=0;
   strncpy(oldunit,header.xunits,11); oldunit[11]=0;
	SetBufferScale( &myBuffer->scaleX, header.xa, header.xb, olddim, oldunit );
   strncpy(olddim,header.ydim,11); olddim[11]=0;
   strncpy(oldunit,header.yunits,11); oldunit[11]=0;
	SetBufferScale( &myBuffer->scaleY, header.ya, header.yb, olddim, oldunit );
   strncpy(olddim,header.idim,11); olddim[11]=0;
   strncpy(oldunit,header.iunits,11); oldunit[11]=0;
	SetBufferScale( &myBuffer->scaleI, header.ia, header.ib, olddim, oldunit );

	// copy the comments. Before copying them to CStrings,
	// add zeros after the last characters in case it has the max. length (40)
   /*
	c1 = header.com1[40], c2 = header.com2[40];		// (= access out of bounds)
	header.com1[40] = 0; itsComment = header.com1; header.com1[40] = c1;
	header.com2[40] = 0;
   if (strlen(header.com2)>0) {
      itsComment += "\n";
      itsComment += header.com2;
   }
   header.com2[40] = c2;
   */

	if (itsVersion > 99)		// old versions; the new numbers lie between 40 and 99
	{	// make old data file formats compatible with version 3.0 and higher
		if ( itsVersion < 210 )								// = version 2.1
		{	header.columns	= 384;	                  // image size
			header.rows		= 286;
			header.xstart  = header.ystart  = 0;		// image shift
			header.com1[0] = header.com2[0] = 0;		// comment
		}
		//if ( itsVersion < 120 )								// = version 1.2
		//	itsDate = itsTime = NULLString;				// date + time
	}

	// set new size and type and allocate pixbuf memory
	//Resize (header.columns, header.rows, IsFloat(), FALSE);
   if (header.imagetype==40)
	{
      fclose(theFile);
      return IMREAD_ERR_FORMAT;
   }
      
   theNF = ((itsVersion>=VER_EXTHEADER && itsVersion<100) ? header.f_dim : 1);
   theNY = (header.rows==-1 ? header.longRows : header.rows);
	theNX = (header.columns==-1 ? header.longColumns : header.columns);
	theNZ = ((itsVersion>=VER_VOLUME_BUFFER && itsVersion<100) ? header.longZDim : 1);

	if ( header.imagetype == IMAGE_SPARSE_WORD || header.imagetype == IMAGE_SPARSE_FLOAT )
	{
      fclose(theFile);
      return IMREAD_ERR_FORMAT;
   }

	if (itsVersion<VER_VOLUME_BUFFER || itsVersion>=100)
	{
		theNY /= theNF;
		CreateBuffer( myBuffer, theNX, theNY, theNZ, theNF, header.imagetype==IMAGE_FLOAT, header.vector_grid, (BufferFormat_t)header.image_sub_type );
	}
	else
	{
		CreateBuffer( myBuffer, theNX,theNY,theNZ,theNF, header.imagetype==IMAGE_FLOAT, header.vector_grid, (BufferFormat_t)header.image_sub_type );
	}

	if (header.imagetype == IMAGE_IMX)				// compressed (IMX) file ?
	{
		ImReadError_t err = SCPackOldIMX_Read(theFile,myBuffer);
		if (err!=IMREAD_ERR_NO)
		{
			fclose(theFile);
			return err;
		}
	}			
	else    // no compression
	{
		for (int row=0; row<myBuffer->totalLines; row++)
		{
			if (myBuffer->isFloat)
				fread( &myBuffer->floatArray[row*myBuffer->nx], sizeof(float), myBuffer->nx, theFile );
			else
				fread( &myBuffer->wordArray[row*myBuffer->nx], sizeof(Word), myBuffer->nx, theFile );
			if (ferror(theFile))
				goto errexit;
		}
	}

   if (myList)
	{
      ReadImgAttributes(theFile,myList);
   }

   fclose(theFile);
   return IMREAD_ERR_NO;

errexit:
   fclose(theFile);
   return IMREAD_ERR_DATA;
}



/*****************************************************************************/
// Read and write attributes
/*****************************************************************************/

typedef enum 
	{
      IEH_END = 0,
      IEH_SCALE_X,
      IEH_SCALE_Y,
      IEH_SCALE_Z,
      IEH_SCALE_I,
      IEH_COMMENT,
      IEH_ATTRIBUTE,
      IEH_SCALE_F,
      IEH_ATTRIBUTE_FLOATARRAY,
      IEH_ATTRIBUTE_INTARRAY,
      IEH_ATTRIBUTE_WORDARRAY,
		IEH_TIME,						// long time with milliseconds, TL 24.10.00
		// jetzt duerfen neue Attribute folgen, die bisherigen Daten versteht auch DaVis 6
		IEH_DATE,
		IEH_MAX = IEH_DATE			// max index of this list
   } type_extheader;

typedef struct
   {  type_extheader type;
      int            size;
   } image_extheader;


int SetAttribute( AttributeList** myList, const char* theName, const char* theValue )
{
	int len;
   AttributeList* ptr = (AttributeList*)malloc(sizeof(AttributeList));
	// create memory or attribute name
	len = strlen(theName);
   ptr->name = (char*)malloc(len+1);
   strcpy(ptr->name,theName);
	ptr->name[len] = '\0';
	// create memory or attribute value
	len = strlen(theValue);
   ptr->value = (char*)malloc(len+1);
   strcpy(ptr->value,theValue);
	ptr->value[len] = '\0';
	// put new item into attribute list
   ptr->next = *myList;
   *myList = ptr;
   return 1;
}


void Attribute_NullToBreak( char* data, int size )
{
	int i;
	for (i=0; i<size; i++)
		if (data[i]=='\0')
			data[i] = '\n';
}

void Attribute_BreakToNull( char* data, int size )
{
	int i;
	for (i=0; i<size; i++)
		if (data[i]=='\n')
			data[i] = '\0';
}


int ReadImgAttributes( FILE* theFile, AttributeList** myList )
{
   image_extheader item;

   while (fread((char*)&item,1,sizeof(item),theFile))
	{
      char* data = NULL;
      if (item.size>0)
		{
         data = (char*)malloc(item.size+1);
         data[item.size] = 0; // final 0-byte for strings
         if (!fread(data,1,item.size,theFile))
		{
			free(data);
            return -1;  // "extended header: no tag data"
		}
      }
		if (data)
		{
			switch (item.type) {
			case IEH_END:
				break;
			case IEH_SCALE_X:
				Attribute_NullToBreak(data,item.size);
				SetAttribute(myList,"_SCALE_X",data);
				break;
			case IEH_SCALE_Y:
				Attribute_NullToBreak(data,item.size);
				SetAttribute(myList,"_SCALE_Y",data);
				break;
			case IEH_SCALE_Z:
				Attribute_NullToBreak(data,item.size);
				SetAttribute(myList,"_SCALE_Z",data);
				break;
			case IEH_SCALE_I:
				Attribute_NullToBreak(data,item.size);
				SetAttribute(myList,"_SCALE_I",data);
				break;
			case IEH_SCALE_F:
				Attribute_NullToBreak(data,item.size);
				SetAttribute(myList,"_SCALE_F",data);
				break;
			case IEH_COMMENT:
				SetAttribute(myList,"_COMMENT",data);
				break;
			case IEH_TIME:
				SetAttribute(myList,"_TIME",data);
				break;
			case IEH_DATE:
				SetAttribute(myList,"_DATE",data);
				break;
			case IEH_ATTRIBUTE:
            {
                char* value_pos = strchr(data,'=');
                if (value_pos)
				{
                    *value_pos = '\0';
                    SetAttribute( myList, data, value_pos+1 );
                }
                break;
            }
            default:
            {
                break;
            }
            }
		    free(data);
		}
   }
   return 0;
}


void WriteAttribute_ITEM( FILE* theFile, type_extheader t, int l, const char* d )
{   
   image_extheader item;
   item.type = t;                            
   item.size = l;                            
   fwrite((char*)&item,sizeof(item),1,theFile);
   fwrite((char*)d,item.size,1,theFile);
	/*
	FILE *f = fopen("c:\\log.txt","a");
	fprintf(f,"Type %i, len=%i: %s\n",(int)t,l,d);
	fclose(f);
	*/
}

void WriteAttribute_SCALE( FILE* theFile, type_extheader t, char* value )
{   
	int len = strlen(value);
	Attribute_BreakToNull( value, len );
	WriteAttribute_ITEM( theFile, IEH_SCALE_X, len, value );
}

void WriteAttribute_END( FILE *theFile )
{
   image_extheader item;
	item.type = IEH_END; 
	item.size = 0; 
	fwrite( (char*)&item, sizeof(item), 1, theFile );
}


int WriteImgAttributes( FILE* theFile, bool isIM6, AttributeList* myList )
{
	AttributeList* itsAttribute = myList;
   while (itsAttribute) 
	{
		if (strcmp(itsAttribute->name,"_SCALE_X")==0)
		{
			WriteAttribute_SCALE( theFile, IEH_SCALE_X, itsAttribute->value );
		}
		else
		if (strcmp(itsAttribute->name,"_SCALE_Y")==0)
		{
			WriteAttribute_SCALE( theFile, IEH_SCALE_Y, itsAttribute->value );
		}
		else
		if (strcmp(itsAttribute->name,"_SCALE_Z")==0)
		{
			WriteAttribute_SCALE( theFile, IEH_SCALE_Z, itsAttribute->value );
		}
		else
		if (strcmp(itsAttribute->name,"_SCALE_I")==0)
		{
			WriteAttribute_SCALE( theFile, IEH_SCALE_I, itsAttribute->value );
		}
		else
		if (strcmp(itsAttribute->name,"_SCALE_F")==0)
		{
			WriteAttribute_SCALE( theFile, IEH_SCALE_F, itsAttribute->value );
		}
		else
		if (strcmp(itsAttribute->name,"_COMMENT")==0)
		{
			WriteAttribute_ITEM( theFile, IEH_COMMENT, strlen(itsAttribute->value), itsAttribute->value );
		}
		else
		if (strcmp(itsAttribute->name,"_TIME")==0)
		{
			WriteAttribute_ITEM( theFile, IEH_TIME, strlen(itsAttribute->value), itsAttribute->value );
		}
		else
		if (strcmp(itsAttribute->name,"_DATE")==0)
		{
			WriteAttribute_ITEM( theFile, IEH_DATE, strlen(itsAttribute->value), itsAttribute->value );
		}
		else
		{
			char* value = (char*)malloc( strlen(itsAttribute->name)+1+strlen(itsAttribute->value)+1 );
			sprintf(value,"%s=%s",itsAttribute->name,itsAttribute->value);
			WriteAttribute_ITEM( theFile, IEH_ATTRIBUTE, strlen(value), value );
			free(value);
		}
		
		itsAttribute = itsAttribute->next;
   }

	WriteAttribute_END(theFile);
   return 0;
}



/*****************************************************************************/
// Write IMG and IMX
/*****************************************************************************/

extern "C" int EXPORT WriteIMG( const char* theFileName, BufferType* myBuffer )
{
	return WriteIMGX( theFileName, false, myBuffer );
}


#define IMX_CODE_WORD	-128
#define IMX_CODE_BIT4	-127
#define IMX_CODE_NWORD	127

inline void Compress16Bits( int n16bytes, signed char*& imx_bpageadr, long& imx_bytecount )
// rearrange last n16bytes in Byte-array to compress things more
{
	int i;
	signed char *adr;
	adr = imx_bpageadr;

	adr -= n16bytes * 3;							/* set back pointer */
	*adr++ = IMX_CODE_NWORD;					/* code for n-16-pixel */
	adr[2] = adr[1];								/* shift first pixel 1 right */
	adr[1] = adr[0];								/* shift first pixel 1 right */
	adr[0] = n16bytes;							/* number of uncompressed pixel */
	adr += 3;										/* increment pointer */	

	// TL 08.03.02: i=0 kopiert in sich selbst
	adr += 2;
#if 1
	for ( i = 1; i < n16bytes-1; i++ )		/* copy rest of pixels */
	{
		*adr = adr[i];								/* left byte of pixel */
		adr++;
		*adr = adr[i];								/* right byte of pixel */
		adr++;										/* increment pointer */
	}
#else
	// TL 08.03.02: dies ist interessanterweise LANGSAMER!
	// laut Aufruf ist immer n16bytes>=3, also die ersten Kopien direkt erzeugen
	// i=1
	*adr = adr[1];
	adr++;
	*adr = adr[1];
	adr++;
	int copycount = n16bytes-3;
	if (copycount>0)
	{	// i=2
		*adr = adr[2];
		adr++;
		*adr = adr[2];
		adr++;
		copycount--;
	}
	if (copycount>0)
	{	// i=3
		*adr = adr[3];
		adr++;
		*adr = adr[3];
		adr++;
		copycount--;
	}
	i = 4;
	while (copycount>0) 
	{	// i=4,5,..
		*adr = adr[i];
		adr++;
		*adr = adr[i];
		adr++;
		copycount--;
		i++;
	}
#endif

	imx_bytecount -= n16bytes - 2;			/* decrement byte counter */
	imx_bpageadr   = adr;
}


bool StoreImxOld( BufferType* myBuffer, int rowFirst, int rowLast, int maxCol, FILE *theFile )
{
	int theNX = myBuffer->nx;

	int BUFFERSIZE	= max( 2048, ((theNX+4096)/512) * 512 );	// n * sectorsize, size of packing memory
	unsigned long BUFFERSAVE = 2 * theNX + 128;	// store memory when only this space is available (may not be enough for the next row)
    char* page;	
	page = (char*)malloc(sizeof(char)*BUFFERSIZE);
	if ( page == NULL )
		return true;

	const Word  *zeile;
	Word        newvalue;
	long	    lastvalueL;
	signed char* imx_bpageadr;
	long		imx_bytecount;
	int         half, nbytes;				/* potential number of 4-bit pixels */
	int         n16bytes;                  	/* number of uncompressed pixels */
	long        diffvalue;
	imx_bpageadr = (signed char*) page;		/* running adress in byte page */
	imx_bytecount = 0;

	lastvalueL = 0;
	n16bytes   = 0;								/* uncompressed pixel */
	
	int row, col;
	for ( row = rowFirst; row < rowLast; row++ )
	{																		/* line in buffer */
		zeile = &myBuffer->wordArray[row*theNX];
		nbytes = 0;														/* 4-bit pixels */
		for ( col = maxCol-1; col>=0; col--, zeile++ )	/* for all pixels */
		{
			newvalue = *zeile;		    							/* get new pixel */
			diffvalue = (long)newvalue - lastvalueL;			/* difference */

			if ( diffvalue > 7 || diffvalue < -7 || col == 0 )
			{																/* check BIT4 mode */
				if ( nbytes >= 7 )									/* Is it worth to */
				{                                            /* use BIT4 mode ? */
					if ( (nbytes % 2) == 0 )                  /* even number ? */
						nbytes--;										/* only use odd number */
					half = nbytes >> 1;
					imx_bpageadr -= nbytes;							/* go backwards */
					imx_bpageadr[1] =                         /* second byte = 2 */
						(imx_bpageadr[0] << 4) | (imx_bpageadr[1]&0x0F); /* pixels */
					imx_bpageadr[0] = IMX_CODE_BIT4;				/* first byte: BIT4 mode */
					for ( int j = 2; j < nbytes-1; j+=2 )     /* recompress rest */
						imx_bpageadr[ 1 + (j>>1) ] = (imx_bpageadr[j] << 4) | (imx_bpageadr[j+1]&0x0F);
					imx_bpageadr[ 1 + half ] =
						(imx_bpageadr[nbytes-1] << 4) | 0x08;	/* last byte + end 0x08 */
					imx_bpageadr += 2 + half;                 /* recompute pointer */
					imx_bytecount -= half - 1;                /* and byte counter */
				}
				nbytes = 0;                                  /* reset counter */
			}
			else
			{
				nbytes++;												/* new potential 4-bit pixel */
			}
			if ( diffvalue > 126 || diffvalue < -126 )      /* too large? */
			{
				*imx_bpageadr++ = (signed char) IMX_CODE_WORD;    /* no compression */
				*((Word*)imx_bpageadr) = newvalue;                /* store pixel: 2-bytes*/
				imx_bpageadr  += 2;                               /* increment pointer */
				imx_bytecount += 3;                               /* 3 new bytes */
				if ( ++n16bytes == 255 )                      /* inc. counter */
				{                                             /* time to store */
					Compress16Bits( n16bytes, imx_bpageadr, imx_bytecount );
					n16bytes = 0;                               /* reset counter */
				}
			}
			else
			{
				if ( n16bytes >= 3 )                          /* is it worth to? */
					Compress16Bits( n16bytes, imx_bpageadr, imx_bytecount );  /* yes, rearrange things */
				n16bytes = 0;                                 /* reset counter */
				*imx_bpageadr++ = (signed char) diffvalue;      /* store only diff. */
				imx_bytecount++;                                /* 1 more byte */
			}
			lastvalueL = newvalue;										/* update lastvalue */
		}									/* endfor columns */

		if ( imx_bytecount > (long)(BUFFERSIZE-BUFFERSAVE) )				/* store line in file */
		{
			if (imx_bytecount != fwrite( page, sizeof(char), imx_bytecount, theFile ))
				goto exiterr;
			imx_bytecount = 0;											/* reset counter */
			imx_bpageadr = (signed char*) page;						/* reset byte pointer */
			nbytes = 0;														/* reset nibble counter */
			n16bytes = 0;													/* reset counter of 16bit */
		}
	}  							/* endfor lines */

	if ( imx_bytecount > 0 )                          /* store rest */
	{
		if (imx_bytecount != fwrite( page, sizeof(char), imx_bytecount, theFile ))
			goto exiterr;
	}
	
	free(page);
	return false;

exiterr:
	free(page); 
	return true;
}


int WriteIMX( FILE *theFile, BufferType* myBuffer )
{
	int theNX = myBuffer->nx,
	    theNY = myBuffer->ny,
		 theNZ = myBuffer->nz,
		 theNF = myBuffer->nf;

// preview:
	int row;
	int steps = max( theNX / 100 + 1, theNY / 100 + 1 );
	Byte ny = (theNY-1) / steps + 1;
	Byte nx = (theNX-1) / steps + 1;
	fwrite( &nx, sizeof(Byte), 1, theFile );			// store 1 byte nx/ny
	fwrite( &ny, sizeof(Byte), 1, theFile );

	Word IntMax = 0;			// maximum intensity of buffer
	// compute maximum:
	for ( row = 0; row < theNY; row += steps )
	{
		const Word* ptr = &myBuffer->wordArray[row*theNX];
		for ( int col = 0; col < theNX; col += steps )
			IntMax = max( ptr[col], IntMax );
	}
	int shiftright = 0;		// get shift value
	while ( (shiftright < 10 ) && (IntMax >= (64 << shiftright)) )
		shiftright++;

// store preview
	const long PreViewSize = 10000;
	Byte* PreViewData = (Byte*)malloc(sizeof(Byte)*PreViewSize);
	int iData = 0;
	for ( row = 0; row < theNY; row += steps )
	{
		const Word* ptr = &myBuffer->wordArray[row*theNX];
		for ( int col = 0; col < theNX; col += steps )
			PreViewData[iData++] = (Byte)(ptr[col] >> shiftright); 
	}
	fwrite( PreViewData, sizeof(Byte), iData, theFile );
	free(PreViewData);

// image data:
	StoreImxOld( myBuffer, 0, theNY*theNZ*theNF, theNX, theFile );

	return IMREAD_ERR_NO;
}


extern "C" int EXPORT WriteIMGX( const char* theFileName, bool isIMX, BufferType* myBuffer )
{
	return WriteIMGXAttr( theFileName, isIMX, myBuffer, NULL );
}

extern "C" int EXPORT WriteIMGXAttr( const char* theFileName, bool isIMX, BufferType* myBuffer, AttributeList* myList )
{
	FILE *theFile = fopen(theFileName,"wb");
	if (theFile==NULL)
		return IMREAD_ERR_FILEOPEN;

	int theNX = myBuffer->nx,
	    theNY = myBuffer->ny,
		 theNZ = myBuffer->nz,
		 theNF = myBuffer->nf;

	// create header
	image_header header;
	header.version		= VER_VOLUME_BUFFER;
	header.imagetype	= (myBuffer->isFloat ? IMAGE_FLOAT : (isIMX?IMAGE_IMX:IMAGE_IMG) );
	header.columns		= theNX;
	header.rows			= theNY;
	header.longZDim	= theNZ;
	header.f_dim		= theNF;
	header.image_sub_type = 0;	// Image

	// copy scales
	header.xa = myBuffer->scaleX.factor;
	header.xb = myBuffer->scaleX.offset;
	strncpy( header.xdim, myBuffer->scaleX.description, 11 );
	strncpy( header.xunits, myBuffer->scaleX.unit, 11 );
	header.ya = myBuffer->scaleY.factor;
	header.yb = myBuffer->scaleY.offset;
	strncpy( header.ydim, myBuffer->scaleY.description, 11 );
	strncpy( header.yunits, myBuffer->scaleY.unit, 11 );
	header.ia = myBuffer->scaleI.factor;
	header.ib = myBuffer->scaleI.offset;
	strncpy( header.idim, myBuffer->scaleI.description, 11 );
	strncpy( header.iunits, myBuffer->scaleI.unit, 11 );

	// create comment
	sprintf( header.com1, "Created by ReadIMX.dll" );
	sprintf( header.com2, " " );

	// store file header contents
	if (!fwrite ((char*)&header, sizeof(header), 1, theFile))
	{
		fclose(theFile);
		return IMREAD_ERR_HEADER;
	}

	// store image data
	int err = IMREAD_ERR_NO;
	if (isIMX && !myBuffer->isFloat)
	{	// Word buffer can be stored as IMX
		err = WriteIMX(theFile,myBuffer);
	}
	else
	{	// simple IMG
		if (myBuffer->isFloat)
		{
			fwrite( myBuffer->floatArray, sizeof(float), theNX*theNY*theNZ*theNF, theFile );
		}
		else
		{
			fwrite( myBuffer->wordArray, sizeof(Word), theNX*theNY*theNZ*theNF, theFile );
		}
	}

	if (myList)
		WriteImgAttributes( theFile, true, myList );

	fclose(theFile);
	return err;
}


