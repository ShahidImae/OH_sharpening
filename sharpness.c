#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h> // For Windows-specific functions
#include <direct.h> // For _mkdir
#include <stdint.h>
#include <errno.h>

// #include "./include/jpeglib.h"
#include <jpeglib.h>

// Function to load a JPEG image (same as before)
unsigned char *load_jpeg(const char *filename, int *width, int *height,
                         int *components) {
    FILE *infile = NULL;
    struct jpeg_decompress_struct cinfo;
    struct jpeg_error_mgr jerr;
    JSAMPARRAY buffer;
    unsigned char *image_data;
    if ((infile = fopen(filename, "rb")) == NULL) {
        perror("Error opening JPEG file");
        return NULL;
    }

    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_decompress(&cinfo);
    jpeg_stdio_src(&cinfo, infile);
    jpeg_read_header(&cinfo, TRUE);

    jpeg_start_decompress(&cinfo);

    *width = cinfo.output_width;
    *height = cinfo.output_height;
    *components = cinfo.output_components;

    image_data = (unsigned char *)malloc(*width * *height * *components);
    if (image_data == NULL) {
        fprintf(stderr,"Error allocating memory for image data\
    ");
        jpeg_finish_decompress(&cinfo);
        jpeg_destroy_decompress(&cinfo);
        fclose(infile);
        return NULL;
    }

    buffer = (*cinfo.mem->alloc_sarray)((j_common_ptr)&cinfo, JPOOL_IMAGE,
                                        *width * *components, 1);

    unsigned char *row_ptr = image_data;
    while (cinfo.output_scanline < cinfo.output_height) {
        jpeg_read_scanlines(&cinfo, buffer, 1);
        for (int i = 0; i < *width * *components; i++) {
            row_ptr[i] = buffer[0][i];
        }
        row_ptr += *width * *components;
    }

    jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);
    fclose(infile);

    return image_data;
}

// Function to save a JPEG image (same as before)
int save_jpeg(const char *filename, unsigned char *image_data, int width,
              int height, int components, int quality) {
    FILE *outfile = NULL;
    struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr jerr;
    JSAMPARRAY buffer;
    int row_stride;
    if ((outfile = fopen(filename, "wb")) == NULL) {
        perror("Error opening output JPEG file");
        return -1;
    }

    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_compress(&cinfo);
    jpeg_stdio_dest(&cinfo, outfile);

    cinfo.image_width = width;
    cinfo.image_height = height;
    cinfo.input_components = components;
    cinfo.in_color_space = components == 3 ? JCS_RGB : JCS_GRAYSCALE;

    jpeg_set_defaults(&cinfo);
    jpeg_set_quality(&cinfo, quality, TRUE);

    jpeg_start_compress(&cinfo, TRUE);

    row_stride = width * components;
    buffer = (*cinfo.mem->alloc_sarray)((j_common_ptr)&cinfo, JPOOL_IMAGE,
                                        row_stride, 1);

    unsigned char *row_ptr = image_data;
    while (cinfo.next_scanline < cinfo.image_height) {
        for (int i = 0; i < row_stride; i++) {
            buffer[0][i] = row_ptr[i];
        }
        jpeg_write_scanlines(&cinfo, buffer, 1);
        row_ptr += row_stride;
    }

    jpeg_finish_compress(&cinfo);
    jpeg_destroy_compress(&cinfo);
    fclose(outfile);

    return 0;
}

// Function to apply a sharpening filter
void laplace_sharpen(unsigned char *input_image, unsigned char *output_image,
                   int width, int height, int components) {
    int i, j, k, x, y;
    int kernel[3][3] = {{-1, -1, -1}, {-1, 9, -1}, {-1, -1, -1}};
    int kernel_sum = 1;
    for (y = 1; y < height - 1; y++) {
        for (x = 1; x < width - 1; x++) {
            for (k = 0; k < components; k++) {
                int pixel_value = 0;
                for (i = -1; i <= 1; i++) {
                    for (j = -1; j <= 1; j++) {
                        int pixel_index = (y + i) * width * components +
                                          (x + j) * components + k;
                        pixel_value +=
                            input_image[pixel_index] * kernel[i + 1][j + 1];
                    }
                }
                pixel_value /= kernel_sum;
                if (pixel_value < 0) pixel_value = 0;
                if (pixel_value > 255) pixel_value = 255;

                output_image[y * width * components + x * components + k] =
                    (unsigned char)pixel_value;
            }
        }
    }

    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x++) {
            if (x == 0 || x == width - 1 || y == 0 || y == height - 1) {
                for (k = 0; k < components; k++) {
                    output_image[y * width * components + x * components + k] =
                        input_image[y * width * components + x * components +
                                    k];
                }
            }
        }
    }
}

void unsharp_mask_sharpen(unsigned char* input_image, unsigned char* output_image,
    int width, int height, int components, float amount) {
    int i, j, k, x, y;
    // 1. 模糊图像 (使用一个简单的均值模糊)
    unsigned char* blurred_image = (unsigned char*)malloc(width * height * components);
    if (!blurred_image) {
        perror("Memory allocation failed");
        return;
    }

    for (y = 1; y < height - 1; y++) {
        for (x = 1; x < width - 1; x++) {
            for (k = 0; k < components; k++) {
                int sum = 0;
                for (i = -1; i <= 1; i++) {
                    for (j = -1; j <= 1; j++) {
                        int pixel_index = (y + i) * width * components +
                            (x + j) * components + k;
                        sum += input_image[pixel_index];
                    }
                }
                blurred_image[y * width * components + x * components + k] =
                    (unsigned char)(sum / 9); // 3x3 均值模糊
            }
        }
    }

    // 复制边缘像素 (模糊操作通常不处理边缘)
    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x++) {
            if (x == 0 || x == width - 1 || y == 0 || y == height - 1) {
                for (k = 0; k < components; k++) {
                    blurred_image[y * width * components + x * components + k] =
                        input_image[y * width * components + x * components + k];
                }
            }
        }
    }

    // 2. 计算差异并 3. 叠加差异
    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x++) {
            for (k = 0; k < components; k++) {
                int original_value = input_image[y * width * components + x * components + k];
                int blurred_value = blurred_image[y * width * components + x * components + k];
                int difference = original_value - blurred_value;
                int sharpened_value = original_value + (int)(amount * difference);

                if (sharpened_value < 0) sharpened_value = 0;
                if (sharpened_value > 255) sharpened_value = 255;

                output_image[y * width * components + x * components + k] =
                    (unsigned char)sharpened_value;
            }
        }
    }

    free(blurred_image);
}

int main() {
    const char *raw_dir = ".\\data\\raw";
    const char *processed_dir = ".\\data\\processed";
    //const char* raw_dir = "D:\\C_Projects\\jpg_sharpening\\jpg_sharpening\\data\\raw";
    //const char* processed_dir = "D:\\C_Projects\\jpg_sharpening\\jpg_sharpening\\data\\processed";
    WIN32_FIND_DATA find_data;
    HANDLE hFind = INVALID_HANDLE_VALUE;
    char search_path[MAX_PATH];
    // Create the processed directory if it doesn't exist
    if (_mkdir(processed_dir) == -1) {
        if (errno != EEXIST) {
            perror("Error creating processed directory");
            return 1;
        }
    }

    // Construct the search path for JPEG files in the raw directory
    snprintf(search_path, MAX_PATH, "%s\\*.jpg", raw_dir);

    hFind = FindFirstFile(search_path, &find_data);

    if (hFind == INVALID_HANDLE_VALUE) {
        snprintf(search_path, MAX_PATH, "%s\\*.jpeg", raw_dir);
        hFind = FindFirstFile(search_path, &find_data);
    }

    if (hFind == INVALID_HANDLE_VALUE) {
        printf("No JPEG files found in the raw directory.\n");
        return 0;
    }

    do {
        if (!(find_data.dwFileAttributes &
              FILE_ATTRIBUTE_DIRECTORY)) {  // Skip directories
            char input_path[MAX_PATH];
            snprintf(input_path, MAX_PATH, "%s\\%s", raw_dir,find_data.cFileName);

            char output_path[MAX_PATH];
            snprintf(output_path, MAX_PATH, "%s\\%s", processed_dir,find_data.cFileName);

            printf("Processing: %s\n",find_data.cFileName);
            int width, height, components;
            unsigned char *image_data =
                load_jpeg(input_path, &width, &height, &components);
            if (image_data == NULL) {
                fprintf(stderr,"Error loading image: %s\n",find_data.cFileName);
                continue;
            }

            unsigned char *sharpened_image =
                (unsigned char *)malloc(width * height * components);
            if (sharpened_image == NULL) {
                fprintf(stderr,"Error allocating memory for sharpened image: %s\n",find_data.cFileName);
                free(image_data);
                continue;
            }
            //sharpen_image(image_data, sharpened_image, width, height, components);
            unsharp_mask_sharpen(image_data, sharpened_image, width, height, components, 1.35f);

            if (save_jpeg(output_path, sharpened_image, width, height,
                          components, 95) != 0) {
                fprintf(stderr,"Error saving sharpened JPEG: %s\n",find_data.cFileName);
            }

            free(image_data);
            free(sharpened_image);
        }
    } while (FindNextFile(hFind, &find_data) != 0);

    FindClose(hFind);

    printf(
        "Finished processing images.\n");
    return 0;
}