
#include <iostream>

#include <jpeglib.h>
#include <jerror.h>

#include "transupp.h"

void setTransformation(jpeg_transform_info *transformObj, JXFORM_CODE transformation){
    transformObj->perfect = FALSE;
    transformObj->trim = FALSE;
    transformObj->force_grayscale = FALSE;
    transformObj->crop = FALSE;
    transformObj->transform = transformation;
}

void releaseRes(j_decompress_ptr srcPtr, j_compress_ptr destPtr){
    
    jpeg_finish_compress(destPtr);
    jpeg_destroy_compress(destPtr);

    (void) jpeg_finish_decompress(srcPtr);
    jpeg_destroy_decompress(srcPtr);
}

void rotateImage(const char *inputFilename, const char *outputFilename, JXFORM_CODE rotationVal){

    FILE *inputFile = fopen(inputFilename, "r");
    if(inputFile==NULL){
        std::cerr<<"ERROR: cannot open input file\n";
        return;
    }

	//Input image will be decompressed into this obj
    struct jpeg_decompress_struct srcObj;
    struct jpeg_error_mgr srcErrMgr;
    
	//Rotated image will be stored in this obj
    struct jpeg_compress_struct destObj;
    struct jpeg_error_mgr destErrMgr;

	//DCT coefficient ptr
    jvirt_barray_ptr *srcCoefArr;
    jvirt_barray_ptr *destCoefArr;

    //Transformation object
    jpeg_transform_info transformObj;

    //Set error handler
    srcObj.err = jpeg_std_error(&srcErrMgr);
    jpeg_create_decompress(&srcObj);

    destObj.err = jpeg_std_error(&destErrMgr);
    jpeg_create_compress(&destObj);

    //Set the transformation properties
    setTransformation(&transformObj, rotationVal);

	//Specifing source of decompression obj
    jpeg_stdio_src(&srcObj, inputFile);
	
	//Enabling default markers to be copied
    JCOPY_OPTION copyOpt = JCOPYOPT_DEFAULT;
    jcopy_markers_setup(&srcObj, copyOpt);

	//reading header of the JPEG image file
    (void) jpeg_read_header(&srcObj, TRUE);

	/* Space needed by the tranformation must be requested before jpeg_read_coefficients()
	 * so that memory allocation will be done properly
	 */
    if(!jtransform_request_workspace(&srcObj, &transformObj)){
        std::cerr<<"Transformation is not perfect\n";
		fclose(inputFile);    
        releaseRes(&srcObj, &destObj);
        return;
    }
	
	//Read source file as DCT coefficient
    srcCoefArr = jpeg_read_coefficients(&srcObj);
	
	//Initializing dest compression obj from source obj
    jpeg_copy_critical_parameters(&srcObj, &destObj);

	/* Adjust dest compression obj, if required by the transformation options
	 * also find out which set of coefficient arrays will hold the output.
	 */
    destCoefArr = jtransform_adjust_parameters(&srcObj, &destObj, srcCoefArr, &transformObj);

	//Opening empty output file, where resulted rotated image will be stored.
    FILE *outputFile = fopen(outputFilename, "wb");
    if(outputFile==NULL){
        std::cerr<<"ERROR: cannot open output file\n";
        fclose(inputFile);    
        releaseRes(&srcObj, &destObj);
        return;
    }

	//Specify the output for compression obj
    jpeg_stdio_dest(&destObj, outputFile);

	//Start compressor and no image data is actually written here.
    jpeg_write_coefficients(&destObj, destCoefArr);

	//Copy to output file any extra marker we want to preserve
    jcopy_markers_execute(&srcObj, &destObj, copyOpt);

    //Starting actual tranformation that is requested (in our case it is rotation) 
    jtransform_execute_transformation(&srcObj, &destObj, srcCoefArr, &transformObj);

	//Releasing resources
    releaseRes(&srcObj, &destObj);
    fclose(inputFile);
    fclose(outputFile);
}

// Driver program
int main(int argc, char *argv[]){
    if(argc < 3){
        std::cerr<<"Please provide the input image and only output image filename\n";
        return -1;
    }

    std::cout<<"--------------------- STRAT ROTATING ------------------------\n";
	/* Avalable options for Rotation:
	 * JXFORM_ROT_90
	 * JXFORM_ROT_180
	 * JXFORM_ROT_270
	 */
    rotateImage(argv[1], argv[2], JXFORM_ROT_90);
    std::cout<<"-------------------------- DONE -----------------------------\n";

    return 0;
}