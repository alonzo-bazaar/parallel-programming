/* SGIL: Stupid Generic Image Library
 * whenever I use stb image libraries half the code I end up writing is
 * "ok if it's a jpeg do this, if it's a png do that, otherwise I don't care"
 * as I just wanna read and write files, but the function to call for that
 * depends on file name
 *
 * so I made this library that's just an overengineered and comically long way to
 * get some image types out of file names and dispatch stb functions on that
 *
 * it's a relatively thin layer around stb image libraries so it should
 * be easy to usee in conjuction to them
 * I hope
 */

// TODO: make function and struct names more prefixy

#ifndef STUPID_GENERIC_IMAGE_LIBRARY_H
#define STUPID_GENERIC_IMAGE_LIBRARY_H

// I'm making this an stb-ish style library
// it's not really self contained as it depends some on stb_*.h being present
// but this is probably gonna be of use to somebody out there so why not
#ifdef SGIL_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#endif

// might as well speed the compilation up
#define STBI_ONLY_JPEG
#define STBI_ONLY_PNG

#include "stb_image.h"
#include "stb_image_write.h"

/* one note for jpegs, we want a unified api for writing png and jpeg images
 * but writing jpegs takes an extra 'image quality' parameter we don't use
 * for pngs
 * so... we're passing it through a global variable :| 
 */
#ifdef SGIL_IMPLEMENTATION
int sgil_jpeg_quality = 80;
# endif

typedef enum {
    SGIL_PNG,
    SGIL_JPEG,
    SGIL_IGNORED,  // image type is known to be present, but ignored
                   // (webp, bitmap, or other formats that ain't png or jpeg)
    SGIL_UNKNOWN   // image type is... not there, and idk what to do about it
} sgil_image_type;

typedef enum {
    SGIL_OK = 0,
    SGIL_IGNORED_FORMAT,
    SGIL_UNKNOWN_FORMAT,
    SGIL_STBI_SIDE_ERROR,
    SGIL_STBIW_SIDE_ERROR,
    SGIL_OTHER_ERROR,
} sgil_exit_status;

typedef struct {
    unsigned char* data;
    int width;
    int height;
    int channels;
    // image write also takes a stride but I don't really pad images
    // so, as of writing, the stride is redundant
    sgil_image_type type;
} sgil_image_handle;

// just an nicer interface than setting a global variable
// that said, all this function does is set a globa variable
void sgil_set_jpeg_quality(int n);

// functions to get file image type out of a file path
// mostly for internal use within the library
const char* sgil_get_file_extension(const char* filename);
sgil_image_type sgil_get_file_image_type(const char* filename);

// used to print function exit codes 
const char* sgil_exit_status_description(const sgil_exit_status es);
// sgil_exit_status return is only used for functions we expect might fail
// that is, read and write opeartations
// I should probably make it an option type with some extra error handling shit

// functions to read images into handles and write images from handles 
sgil_exit_status sgil_read_image(const char* filename, sgil_image_handle* ih);
sgil_exit_status sgil_write_image(const char* filename, sgil_image_handle* ih);

// functions to free images, given handles
// use this if you allocated the handle statically
// (sgil_image_handle h = {0};)
void sgil_free_handle_data(sgil_image_handle* ih);
// use this if you allocated the handle dymaically
// (sgil_image_handle h* = (sgil_image_handle*) malloc(sizeof(sgil_image_handle));)
void sgil_free_handle(sgil_image_handle* ih);

// convenience macro
// you could just as easily do if(<sgil_call> != 0) exit(1); if you don't
// want too much logging, but I do quite enjoy too much logging
#define SGIL_TRY(call) do{						\
    sgil_exit_status status = call;					\
    switch(status) {							\
    case SGIL_OK:							\
	(void)0;							\
	break;								\
    case SGIL_STBI_SIDE_ERROR:						\
	fprintf(stderr, "SGIL STBI SIDE ERROR at line %d:\n", __LINE__ ); \
	fprintf(stderr, "\n%s\n", stbi_failure_reason());		\
	exit(1);							\
    default:								\
	fprintf(stderr, "SGIL ERROR at line %d:\n%s\n",			\
		__LINE__,						\
		sgil_exit_status_description(status));			\
	exit(1);							\
    }									\
    }while(0)

#ifdef SGIL_IMPLEMENTATION
/*
 * to have a nicer interface than just setting a global variable
 */
void
sgil_set_jpeg_quality(int n) {
    sgil_jpeg_quality = n;
}

/*
 * returns slice of string starting from last dot '.' up until end of string
 * if no dot is found, or if the string starts with a dot
 * it returns the entire string
 */
const char*
sgil_get_file_extension(const char* filename) {
    const char* end = filename + strlen(filename) - 1; // last char in name
    while(end != filename && *end != '.') end--;
    return end;
}
/*
 * determines image type of image called `filename' by the file extension
 * found threin, uses `get_file_extension'
 */
sgil_image_type
sgil_get_file_image_type(const char* filename) {
    const char* c = sgil_get_file_extension(filename);
    if(c == filename)
	// no extension found
	return SGIL_UNKNOWN;
    else if((strcmp(c, ".jpg") == 0) || (strcmp(c, ".jpg") == 0))
	return SGIL_JPEG;
    else if(strcmp(c, ".png") == 0)
	return SGIL_PNG;
    else
	// file extension found but ignored
	return SGIL_OTHER_ERROR;
}

/*
 * get human readable description of exit status,
 * mostly useful for print debugging and error messages to the programmer
 */
const char*
sgil_exit_status_description(const sgil_exit_status es) {
    switch(es) {
    case SGIL_OK:
	return "function call finished succesfully";
    case SGIL_IGNORED_FORMAT:
	return "purposefully ignored image format";
    case SGIL_UNKNOWN_FORMAT:
	return "unknown image format";
    case SGIL_STBI_SIDE_ERROR:
	return "error occured during stb image call,"
	    " see what stb image has to say";
    case SGIL_STBIW_SIDE_ERROR:
	return "error occured during stb image write call,"
	    " see what stb image write has to say";
    case SGIL_OTHER_ERROR:
	return "some unknown error, you're on you own kid";
    default:
	return "I have no idea how did you get here,"
	    " this error handling function wasn't supposed"
	    " to error out, this shit is cursed"
	    " good luck bro";
    }
}

// hic sunt stbiw stbique

/*
 * reads image file `filename' into image handle `ih'
 * if return code is not SGIL_OK then `ih' has not been altered
 * and using it will likely cause a segfault, or worse, a silent failure
 * (but it will most likely just segfault)
 */
sgil_exit_status
sgil_read_image(const char* filename, sgil_image_handle* ih) {
    sgil_image_type it = sgil_get_file_image_type(filename);
    if(it==SGIL_IGNORED) return SGIL_IGNORED_FORMAT;
    if(it==SGIL_UNKNOWN) return SGIL_UNKNOWN_FORMAT;

    int w, h, c; // width, height, channels
    int ok = stbi_info(filename, &w, &h, &c);
    if(!ok) return SGIL_UNKNOWN_FORMAT;

    unsigned char* data = stbi_load(filename, &w, &h, &c, 0);
    if(data == NULL) return SGIL_STBI_SIDE_ERROR;

    ih->data = data;
    ih->width = w;
    ih->height = h;
    ih->channels = c;
    ih->type = it;
    return SGIL_OK;
}

// this function is the entire reason I made this stupid wrapper

/*
 * writes image stored in `ih' to file `filename'
 * the type of image written is determined by file extension in filename
 * and does not need to match the type of image that `ih' is pointing to
 *
 * image is assumed to have no padding, that is, image stride
 * (the amount of bytes in an a row of pixels) is assumed to be equal to
 * ih->channels*ih->width
 *
 * if `filename' has a malformed, erroneous, lacking, or purposefully ignored
 * file extension then the function exits with an error and won't write file
 */
sgil_exit_status
sgil_write_image(const char* filename, sgil_image_handle* ih) {
    switch(sgil_get_file_image_type(filename)) {
    case SGIL_PNG:
	if(stbi_write_png(filename,
			  ih->width, ih->height, ih->channels,
			  ih->data, (ih->channels)*(ih->width)))
	    return SGIL_OK;
	else
	    return SGIL_STBIW_SIDE_ERROR;
    case SGIL_JPEG:
	if(stbi_write_jpg(filename,
			  ih->width, ih->height, ih->channels,
			  ih->data, sgil_jpeg_quality))
	    return SGIL_OK;
	else
	    return SGIL_STBIW_SIDE_ERROR;

    case SGIL_IGNORED:
	return SGIL_IGNORED_FORMAT;
    case SGIL_UNKNOWN:
	return SGIL_UNKNOWN_FORMAT;
    default:
	return SGIL_OTHER_ERROR;
    }
}

/*
 * deallocates image buffer held by the handle
 */
void
sgil_free_handle_data(sgil_image_handle* ih) {
    stbi_image_free(ih->data);
}

/*
 * deallocates handle and image buffer held by the handle
 * do not call on statically allocated handles, or it will segfault
 */
void
sgil_free_handle(sgil_image_handle* ih) {
    stbi_image_free(ih->data);
    free(ih);
}

#endif // SGIL_IMPLEMENTATION
#endif // STUPID_GENERIC_IMAGE_LIBRARY_H
