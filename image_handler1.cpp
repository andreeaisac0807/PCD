#include "image_handler1.h"
#include "image_processing1.h"
#include <opencv2/opencv.hpp>
#include <vector>
#include <string>
#include <unistd.h>
#include <sys/socket.h>
#include <iostream>  // Include iostream for printing

using namespace cv;

void handle_image_processing(const unsigned char* img_data, size_t img_size, const char* name, int client_socket) {
    // Print statement to indicate the start of the function
    std::cout << "sunt la inceputul functiei" << std::endl;

    // Decode the image
    std::vector<unsigned char> img_vector(img_data, img_data + img_size);
    Mat img = imdecode(img_vector, IMREAD_COLOR);

    // Check if the image was successfully decoded
    if (img.empty()) {
        std::cerr << "Failed to decode the image." << std::endl;
        return;
    }

    std::cout << "sunt dupa decoadare" << std::endl;

    // Save the received image to a file
    std::string input_file = std::string(name) + ".jpg";
    if (!imwrite(input_file, img)) {
        std::cerr << "Failed to write the image to " << input_file << std::endl;
        return;
    }

    std::cout << "sunt dupa salvare" << std::endl;

    // Prepare the output file name
    std::string output_file = "processed_" + std::string(name) + ".png";
    std::cout << "sunt dupa procesare " << output_file << std::endl;

    // Process the image
    process_image(input_file, output_file, name);
    std::cout << "sunt dupa a doua procesare" << std::endl;

    // Send the processed image back to the client
    Mat processed_img = imread(output_file, IMREAD_COLOR);

    // Check if the processed image was successfully loaded
    if (processed_img.empty()) {
        std::cerr << "Failed to load the processed image from " << output_file << std::endl;
        return;
    }

    std::vector<unsigned char> processed_img_vector;
    imencode(".png", processed_img, processed_img_vector);
    int processed_img_size = processed_img_vector.size();
    send(client_socket, &processed_img_size, sizeof(int), 0);
    send(client_socket, processed_img_vector.data(), processed_img_size, 0);

    std::cout << "sunt la final" << std::endl;
}
