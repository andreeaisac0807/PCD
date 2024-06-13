
#include "image_processing1.h"
#include <opencv2/opencv.hpp>
#include <fstream>
#include <jsoncpp/json/json.h>

using namespace cv;

void detect_circles(Mat& img, std::vector<Vec3f>& circles) {
    Mat gray;
    cvtColor(img, gray, COLOR_BGR2GRAY);
    GaussianBlur(gray, gray, Size(9, 9), 2, 2);
    HoughCircles(gray, circles, HOUGH_GRADIENT, 1, gray.rows / 8, 100, 30, 20, 40);
}

void process_image(const std::string& input_path, const std::string& output_path, const std::string& email) {
    // Load images
    Mat img = imread(input_path, IMREAD_COLOR);
    Mat correct_img = imread("corect.jpg", IMREAD_COLOR);

    // Detect circles
    std::vector<Vec3f> circles_img, circles_correct;
    detect_circles(img, circles_img);
    detect_circles(correct_img, circles_correct);

    int score = 0;
    int total_questions = circles_correct.size();

    for (const auto& correct_circle : circles_correct) {
        Point center(cvRound(correct_circle[0]), cvRound(correct_circle[1]));
        int radius = cvRound(correct_circle[2]);
        bool found = false;
        for (const auto& user_circle : circles_img) {
            Point user_center(cvRound(user_circle[0]), cvRound(user_circle[1]));
            if (norm(center - user_center) < radius) {
                found = true;
                break;
            }
        }
        Scalar color = found ? Scalar(0, 255, 0) : Scalar(0, 0, 255);
        circle(img, center, radius, color, 2);
        if (found) {
            score++;
        }
    }

    int percentage = total_questions > 0 ? (score * 100) / total_questions : 0;

    // Draw the score on the image
    putText(img, std::to_string(percentage) + "%", Point(50, 400), FONT_HERSHEY_SIMPLEX, 3, Scalar(0, 255, 0), 5);

    // Save the processed image
    imwrite(output_path, img);

    // Update results.json
    Json::Value results;

    // Try to open the existing results.json file
    std::ifstream file("results.json");
    if (file.is_open()) {
        file >> results;
        file.close();
    } else {
        // If the file does not exist, initialize an empty JSON object
        results = Json::Value(Json::objectValue);
    }

    results[email] = percentage;

    std::ofstream out_file("results.json");
    out_file << results;
    out_file.close();
}
