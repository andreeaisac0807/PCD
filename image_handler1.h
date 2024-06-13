#ifndef IMAGE_HANDLER_H
#define IMAGE_HANDLER_H

#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

void handle_image_processing(const unsigned char* img_data, size_t img_size, const char* name, int client_socket);

#ifdef __cplusplus
}
#endif

#endif
