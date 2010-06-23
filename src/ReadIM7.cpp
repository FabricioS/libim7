/* ReadIM7
	TL 05.01.04

	Read a LaVision IM7/VC7 file by a call to:

   extern "C" int EXPORT ReadIM7 ( const char* theFileName, BufferType* myBuffer, AttributeList** myList );

   If you don't want to read the attributes, set myList = NULL. If myList is set,
   the parameter returns a pointer to the list. You have to free the strings manually!

   The function returns the error codes of ImReadError_t.
*/

#include "ReadIMX.h"
#include "ReadIM7.h"
#include<zlib.h>

enum IM7PackType_t
{
	IM7_PACKTYPE__UNCOMPR= 0x1000,// autoselect uncompressed
	IM7_PACKTYPE__FAST,				// autoselect fastest packtype, 
	IM7_PACKTYPE__SIZE,				// autoselect packtype with smallest resulting file
	IM7_PACKTYPE_IMG			= 0,	// uncompressed, like IMG
	IM7_PACKTYPE_IMX,					// old version compression, like IMX
	IM7_PACKTYPE_ZLIB,				// zlib
	IM7_PACKTYPE_FIXED_12_0,		// 12 bit format without bitshift
};


ImReadError_t SCPackUncompressed_Read( FILE* theFile, BufferType* myBuffer, bool p_bBytes )
{
	for (int row=0; row<myBuffer->totalLines; row++)
	{
		unsigned long destLen = 0;
		Byte *dest = Buffer_GetRowAddrAndSize(myBuffer,row,destLen);

		if (p_bBytes)
		{	// read row of bytes
			destLen /= 2;
			size_t n = fread(dest,1,destLen,theFile);
			if (n!=destLen || ferror(theFile))
				return IMREAD_ERR_DATA;
			// copy bytes into the words
			Word *pWord = (Word*)dest;
			for (int i=destLen-1; i>=0; i--)
				pWord[i] = dest[i];
		}
		else
		{	// read row of words
			size_t n = fread(dest,1,destLen,theFile);
			if (n!=destLen || ferror(theFile))
				return IMREAD_ERR_DATA;
		}
	}
	return IMREAD_ERR_NO;
}



ImReadError_t SCPackZlib_Read( FILE* theFile, BufferType* myBuffer )
{
	int sourceLen = 0;
	Bytef *source = NULL;

	fread( &sourceLen, sizeof(sourceLen), 1, theFile );
	source = (Byte*)malloc(sourceLen);
	if (source==NULL)
	{
		return IMREAD_ERR_MEMORY;
	}
	fread( source, 1, sourceLen, theFile );
	
	uLongf destLen = 0;
	Bytef *dest = Buffer_GetRowAddrAndSize(myBuffer,0,destLen);
	destLen *= myBuffer->totalLines;

	int err = uncompress( dest, &destLen, source, sourceLen );

	ImReadError_t errret = IMREAD_ERR_NO;
	switch (err)
	{
		case Z_OK:
			break;
		case Z_MEM_ERROR:
			errret = IMREAD_ERR_MEMORY;
			break;
		case Z_BUF_ERROR:
			//"Output buffer too small for uncompressed data!"
			errret = IMREAD_ERR_MEMORY;
			break;
		case Z_DATA_ERROR:
			// "Compressed data is corrupt!"
			errret = IMREAD_ERR_DATA;
			break;
	}

	free(source);
	return errret;
}



ImReadError_t SCPackFixedBits_Read( FILE* theFile, BufferType* myBuffer, int theValidBits )
{
	if (theValidBits<= 8)
		theValidBits =  8;
	else
//	if (theValidBits<=10)
//		theValidBits = 10;
//	else
	if (theValidBits<=12)
		theValidBits = 12;
//	else
//	if (theValidBits<=14)
//		theValidBits = 14;
	else
		theValidBits = 16;

	Word *dataPtr;
	Word array[16];

	for (int row=0; row<myBuffer->totalLines; row++)
	{
		unsigned long destLen = 0;
		dataPtr = (Word*) Buffer_GetRowAddrAndSize(myBuffer,row,destLen);
		int nx = myBuffer->nx;
		while (nx>0)
		{
			switch (theValidBits)
			{
				case 8:
					fread( array, sizeof(Word), 1, theFile );
					if (nx<2)
					{	// last pixel of a row
						*dataPtr = (array[0] & 0x00FF);
						dataPtr++;
						nx = 0;
					}
					else
					{
						*dataPtr =  (array[0] & 0x00FF);
						dataPtr++;
						*dataPtr = ((array[0] & 0xFF00) >> 8);
						dataPtr++;
						nx -= 2;
					}
					break;
				case 10:
					break;
				case 12:
					if (nx<4)
					{	// last pixel of a row
						fread( dataPtr, sizeof(Word), nx, theFile );
						nx = 0;
					}
					else
					{	// decompress next 4 pixel
						fread( array, sizeof(Word), 3, theFile );
						*dataPtr = (array[0] & 0x0FFF);
						dataPtr++;
						*dataPtr = (array[0] >> 12) | ((array[1] & 0x00FF) << 4);
						dataPtr++;
						*dataPtr = ((array[1] & 0xFF00) >> 8) | ((array[2] & 0x000F) << 8);
						dataPtr++;
						*dataPtr = (array[2] >> 4);
						dataPtr++;
						nx -= 4;
					}
					break;
				case 14:
					break;
				case 16:
					// read complete line in one step
					fread( dataPtr, sizeof(Word), nx, theFile );
					nx = 0;
					break;
			}
		}
	}
	return IMREAD_ERR_NO;
}



void Scale_Read( const char*theData, BufferScaleType* theScale )
{
	int pos;
	sscanf(theData,"%f %f%n",&theScale->factor,&theScale->offset,&pos);
	theScale->unit[0] = 0;
	theScale->description[0] = 0;
	if (pos>0)
	{
		while (theData[pos]==' ' || theData[pos]=='\n')
			pos++;
		strncpy( theScale->unit, theData+pos, sizeof(theScale->unit) );
		theScale->unit[sizeof(theScale->unit)-1] = 0;
		pos++;
		while (theData[pos]!=' ' && theData[pos]!='\n' && theData[pos]!='\0')
			pos++;
		while (theData[pos]==' '|| theData[pos]=='\n')
			pos++;
		strncpy( theScale->description, theData+pos, sizeof(theScale->description) );
		theScale->description[sizeof(theScale->description)-1] = 0;
		// cut unit
		pos = 0;
		while (theScale->unit[pos]!=' ' && theScale->unit[pos]!='\n' && theScale->unit[pos]!='\0')
			pos++;
		theScale->unit[pos] = '\0';
	}
}


extern "C" int EXPORT ReadIM7 ( const char* theFileName, BufferType* myBuffer, AttributeList** myList )
{
	FILE* theFile = fopen(theFileName, "rb");				// open for binary read
	if (theFile==NULL)
	   return IMREAD_ERR_FILEOPEN;

	// Read an image in our own IMX or IMG or VEC or VOL format
	int theNX,theNY,theNZ,theNF;
	// read and store file header contents
	Image_Header_7 header;
	if (!fread ((char*)&header, sizeof(header), 1, theFile))
	{
      fclose(theFile);
      return IMREAD_ERR_HEADER;
   }

	switch (header.version)
	{
		case IMAGE_IMG:
		case IMAGE_IMX:
		case IMAGE_FLOAT:
		case IMAGE_SPARSE_WORD:
		case IMAGE_SPARSE_FLOAT:
		case IMAGE_PACKED_WORD:
			fclose(theFile);
			return ReadIMX(theFileName,myBuffer,myList);
	}

	if (header.isSparse)
	{
      fclose(theFile);
      return IMREAD_ERR_FORMAT;
	}

	theNX = header.sizeX;
	theNY = header.sizeY;
	theNZ = header.sizeZ;
	theNF = header.sizeF;
	if (header.buffer_format > 0)
	{	// vector
		const int compN[] = { 1, 9, 2, 10, 3, 14 };
		theNY *= compN[header.buffer_format];
	}
	bool bFloat = header.buffer_format!=BUFFER_FORMAT_WORD && header.buffer_format!=BUFFER_FORMAT_MEMPACKWORD;
	CreateBuffer( myBuffer, theNX,theNY,theNZ,theNF, bFloat, header.vector_grid, (BufferFormat_t)header.buffer_format );

	ImReadError_t errret = IMREAD_ERR_NO;
	//fprintf(stderr,"format=%i pack=%i\n",header.buffer_format,header.pack_type);
	switch (header.pack_type)
	{
		case IM7_PACKTYPE_IMG:
			errret = SCPackUncompressed_Read( theFile, myBuffer, header.buffer_format==BUFFER_FORMAT_MEMPACKWORD );
			break;
		case IM7_PACKTYPE_IMX:
			errret = SCPackOldIMX_Read(theFile,myBuffer);
			break;
		case IM7_PACKTYPE_ZLIB:
			errret = SCPackZlib_Read(theFile,myBuffer);
			break;
		case IM7_PACKTYPE_FIXED_12_0:
			errret = SCPackFixedBits_Read( theFile, myBuffer, 12 );
			break;
		default:
			errret = IMREAD_ERR_FORMAT;
	}

	if (errret==IMREAD_ERR_NO)
	{
		AttributeList* tmpAttrList = NULL;
		AttributeList** useList = (myList!=NULL ? myList : &tmpAttrList);
      ReadImgAttributes(theFile,useList);
		AttributeList* ptr = *useList;
		while (ptr!=NULL)
		{
			//fprintf(stderr,"%s: %s\n",ptr->name,ptr->value);
			if (strncmp(ptr->name,"_SCALE_",7)==0)
			{
				switch (ptr->name[7])
				{
					case 'X':	Scale_Read( ptr->value, &myBuffer->scaleX );	break;
					case 'Y':	Scale_Read( ptr->value, &myBuffer->scaleY );	break;
					case 'I':	Scale_Read( ptr->value, &myBuffer->scaleI );	break;
				}
			}
			ptr = ptr->next;
		}
   }

	fclose(theFile);
	return errret;
}


extern "C" int EXPORT WriteIM7 ( const char* theFileName, bool isPackedIMX, BufferType* myBuffer )
{
	int theNX = myBuffer->nx,
	    theNY = myBuffer->ny,
		 theNZ = myBuffer->nz,
		 theNF = myBuffer->nf;

	FILE* theFile = fopen(theFileName, "wb");				// open for binary write
	if (theFile==NULL)
	   return IMREAD_ERR_FILEOPEN;

	Image_Header_7 header;
	header.version				= 0;
	header.isSparse			= 0;
	header.sizeX				= theNX;
	header.sizeY				= theNY;
	header.sizeZ				= theNZ;
	header.sizeF				= theNF;
	header.scalarN				= 0;
	header.vector_grid	 	= 1;
	header.extraFlags			= 0;
	header.buffer_format 	= (myBuffer->isFloat ? -3 : -4 );
	if (myBuffer->isFloat)
		isPackedIMX = false;
	header.pack_type			= (isPackedIMX ? IM7_PACKTYPE_IMX : IM7_PACKTYPE_IMG);
	if (fwrite( &header, sizeof(header), 1, theFile )!=1)
	{
		fclose(theFile);
		return IMREAD_ERR_HEADER;
	}

	// write data
	if (isPackedIMX)
	{
		int err = WriteIMX( theFile, myBuffer );
		if (err)
		{
			fclose(theFile);
			return IMREAD_ERR_DATA;
		}
	}
	else
	{
		for (int row=0; row<theNY*theNZ*theNF; row++)
		{
			if (myBuffer->isFloat)
				fwrite( &myBuffer->floatArray[row*theNX], sizeof(float), theNX, theFile );
			else
				fwrite( &myBuffer->wordArray[row*theNX], sizeof(Word), theNX, theFile );
		}
	}

	// write attributes
	WriteAttribute_END(theFile);
	
	fclose(theFile);
	return 0;
}


/*****************************************************************************/
// LabView functions
/*****************************************************************************/

#ifdef _WIN32

BufferType		LabViewBuffer;
AttributeList*	LabViewAttributeList = NULL;

extern "C" int EXPORT LabView_OpenIMX ( const char* theFileName, int* theWidth, int* theHeight )
{
	DestroyAttributeList(&LabViewAttributeList);
	if (ReadIM7(theFileName,&LabViewBuffer,&LabViewAttributeList)==0)
	{
		*theWidth  = LabViewBuffer.nx;
		*theHeight = LabViewBuffer.ny * LabViewBuffer.nf;
		return (int)&LabViewBuffer;
	}
	*theWidth  = 0;
	*theHeight = 0;
	return 0;
}

extern "C" int EXPORT LabView_ReadIMX_u16 ( int theHandle, Word* theArray )
{
	int x,y,fr;
	if (LabViewBuffer.isFloat)
	{
		float* ptr = LabViewBuffer.floatArray;
		for (fr=0; fr<LabViewBuffer.nf; fr++)
		{
			for (y=0; y<LabViewBuffer.ny; y++)
			{
				for (x=0; x<LabViewBuffer.nx; x++, theArray++, ptr++)
				{
					*theArray = (Word) min( max(0,*ptr), 65535 );
				}
			}
		}
	}
	else
	{
		memcpy( theArray, LabViewBuffer.wordArray, sizeof(Word)*LabViewBuffer.nx*LabViewBuffer.ny*LabViewBuffer.nf );
	}
	return 0;
}

extern "C" int EXPORT LabView_ReadIMX_f32 ( int theHandle, float* theArray )
{
	int x,y,fr;
	if (!LabViewBuffer.isFloat)
	{
		Word* ptr = LabViewBuffer.wordArray;
		for (fr=0; fr<LabViewBuffer.nf; fr++)
		{
			for (y=0; y<LabViewBuffer.ny; y++)
			{
				for (x=0; x<LabViewBuffer.nx; x++, theArray++, ptr++)
				{
					*theArray = *ptr;
				}
			}
		}
	}
	else
	{
		memcpy( theArray, LabViewBuffer.floatArray, sizeof(float)*LabViewBuffer.nx*LabViewBuffer.ny*LabViewBuffer.nf );
	}
	return 0;
}

extern "C" int EXPORT LabView_CloseIMX( int theHandle )
{
	DestroyBuffer(&LabViewBuffer);
	
	// don't destroy attribute lst for later call to Write funtions, but destroy after Write
	if (theHandle==0)
		DestroyAttributeList(&LabViewAttributeList);

	return 0;
}

void LabView_CheckWriteAttr( int& theSizeY )
{
	if (theSizeY>=0)
	{	// don't store attributes
		DestroyAttributeList(&LabViewAttributeList);
	}
	else
	{	// leave attributes for storage
		theSizeY = -theSizeY;
	}
}

extern "C" int EXPORT LabView_WriteIMX_u16 ( const char* theFileName, const Word* theArray, int theSizeX, int theSizeY )
{
	LabView_CheckWriteAttr(theSizeY);
	CreateBuffer( &LabViewBuffer, theSizeX, theSizeY, 1, 1, false, 1, BUFFER_FORMAT_IMAGE );
	memcpy( LabViewBuffer.wordArray, theArray, sizeof(Word)*LabViewBuffer.nx*LabViewBuffer.ny );
	
	int err = WriteIMGXAttr( theFileName, true, &LabViewBuffer, LabViewAttributeList );
	LabView_CloseIMX(0);
	return err;
}

extern "C" int EXPORT LabView_WriteIMXframes_u16 ( const char* theFileName, const Word* theArray, int theSizeX, int theSizeY, int theFrames )
{
	LabView_CheckWriteAttr(theSizeY);
	CreateBuffer( &LabViewBuffer, theSizeX, theSizeY, 1, theFrames, false, 1, BUFFER_FORMAT_IMAGE );
	memcpy( LabViewBuffer.wordArray, theArray, sizeof(Word)*LabViewBuffer.nx*LabViewBuffer.ny*LabViewBuffer.nf );
	
	int err = WriteIMGXAttr( theFileName, true, &LabViewBuffer, LabViewAttributeList );
	LabView_CloseIMX(0);
	return err;
}

extern "C" int EXPORT LabView_WriteIMX_f32 ( const char* theFileName, const float* theArray, int theSizeX, int theSizeY )
{
	LabView_CheckWriteAttr(theSizeY);
	CreateBuffer( &LabViewBuffer, theSizeX, theSizeY, 1, 1, true, 1, BUFFER_FORMAT_IMAGE );
	memcpy( LabViewBuffer.floatArray, theArray, sizeof(float)*LabViewBuffer.nx*LabViewBuffer.ny );
	
	int err = WriteIMGXAttr( theFileName, true, &LabViewBuffer, LabViewAttributeList );
	LabView_CloseIMX(0);
	return err;
}

extern "C" int EXPORT LabView_WriteIMXframes_f32 ( const char* theFileName, const float* theArray, int theSizeX, int theSizeY, int theFrames )
{
	LabView_CheckWriteAttr(theSizeY);
	CreateBuffer( &LabViewBuffer, theSizeX, theSizeY, 1, theFrames, true, 1, BUFFER_FORMAT_IMAGE );
	memcpy( LabViewBuffer.floatArray, theArray, sizeof(float)*LabViewBuffer.nx*LabViewBuffer.ny*LabViewBuffer.nf );
	
	int err = WriteIMGXAttr( theFileName, true, &LabViewBuffer, LabViewAttributeList );
	LabView_CloseIMX(0);
	return err;
}


#endif
