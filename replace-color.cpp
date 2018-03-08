#include <dlib/dir_nav/dir_nav_extensions.h>
#include <opencv2/imgcodecs/imgcodecs.hpp>
#include "cxxopts/include/cxxopts.hpp"

const auto numeric_to_color = [](unsigned long numeric) {
    cv::Vec4b color;
    color[0] = (numeric >> 8)  & 0xff; // blue
    color[1] = (numeric >> 16) & 0xff; // green
    color[2] = (numeric >> 24) & 0xff; // red
    color[3] = (numeric >> 0)  & 0xff; // alpha
    return color;
};

const auto color_to_numeric = [](const cv::Vec4b& color) {
    return (static_cast<unsigned long>(color[0]) << 8)  // blue
         | (static_cast<unsigned long>(color[1]) << 16) // green
         | (static_cast<unsigned long>(color[2]) << 24) // red
         |  static_cast<unsigned long>(color[3]);       // alpha
};

int main(int argc, char** argv)
{
    if (argc == 1) {
        std::cout << "Usage: " << std::endl;
        std::cout << "> replace-color -d=/path/to/images -s=.png -f=0xffff00ff -t=0xffff0080" << std::endl;
        return 1;
    }

    cxxopts::Options options("replace-color", "Replace a single color in a bunch of RGBA images");

    options.add_options()
        ("d,directory", "The directory where to search for input files", cxxopts::value<std::string>())
        ("s,filename-suffix", "How the input file names should end", cxxopts::value<std::string>())
        ("f,from-color", "Which RGBA color to change; for example, try 0xffff00ff for yellow", cxxopts::value<std::string>())
        ("t,to-color", "Which RGBA color to change to; for example, try 0xffff0080 for yellow with alpha", cxxopts::value<std::string>())
        ("c,show-colors", "Show colors actually found?")
        ;

    try {
        options.parse(argc, argv);

        cxxopts::check_required(options, { "directory", "filename-suffix", "from-color", "to-color" });

        const std::string directory         = options["directory"]      .as<std::string>();
        const std::string filename_suffix   = options["filename-suffix"].as<std::string>();
        const std::string from_color_string = options["from-color"]     .as<std::string>();
        const std::string to_color_string   = options["to-color"]       .as<std::string>();        
        const bool show_colors              = options.count("show-colors") > 0;

        unsigned long from_color_numeric = std::stoul(from_color_string, nullptr, 16);
        unsigned long to_color_numeric   = std::stoul(to_color_string, nullptr, 16);

        const cv::Vec4b from_color = numeric_to_color(from_color_numeric);
        const cv::Vec4b to_color = numeric_to_color(to_color_numeric);

        assert(from_color_numeric == color_to_numeric(from_color));
        assert(to_color_numeric == color_to_numeric(to_color));

        const auto color_to_string = [](const cv::Vec4b& color) {
            std::ostringstream oss;
            oss << "RGBA = ("
                << "0x" << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(color[2]) << ", "
                << "0x" << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(color[1]) << ", "
                << "0x" << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(color[0]) << ", "
                << "0x" << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(color[3]) << ")";
            return oss.str();
        };

        std::cout << "Converting from : " << color_to_string(from_color) << std::endl;
        std::cout << "             to : " << color_to_string(to_color) << std::endl;

        std::cout << "  Searching for : *" << filename_suffix << std::endl;
        std::cout << "             in : " << directory << std::endl;

        const std::vector<dlib::file> files = dlib::get_files_in_directory_tree(directory,
            [&filename_suffix](const dlib::file& name) {
            return dlib::match_ending(filename_suffix)(name);
        });

        std::cout << "Found " << files.size() << " files, now converting ..." << std::endl;

        size_t total_converted_pixel_count = 0;
        size_t converted_file_count = 0;

        for (const auto& file : files) {

            const std::string& full_name = file.full_name();

            std::cout << "Processing " << full_name;

            cv::Mat image = cv::imread(full_name, cv::IMREAD_UNCHANGED);

            if (!image.data) {
                std::cout << " - unable to read, skipping...";
            }
            else {
                std::cout << std::dec
                    << ", width = " << image.cols
                    << ", height = " << image.rows
                    << ", channels = " << image.channels()
                    << ", type = 0x" << std::hex << image.type();

                if (image.channels() != 4) {
                    std::cout << " - need 4 channels, skipping...";
                }
                else {
                    struct compare_color {
                        bool operator() (const cv::Vec4b& a, const cv::Vec4b &b) const {
                            return color_to_numeric(a) < color_to_numeric(b);
                        }
                    };
                    std::set<cv::Vec4b, compare_color> colors_found;
                    size_t converted_pixel_count = 0;
                    for (int y = 0; y < image.rows; ++y) {
                        cv::Vec4b* row = image.ptr<cv::Vec4b>(y);
                        for (int x = 0; x < image.cols; ++x) {
                            cv::Vec4b& color = row[x];
                            if (color == from_color) {
                                color = to_color;
                                ++converted_pixel_count;
                            }
                            if (show_colors) {
                                colors_found.insert(color);
                            }
                        }
                    }
                    std::cout << ": converted " << std::dec << converted_pixel_count << " pixels";
                    if (converted_pixel_count > 0) {
                        cv::imwrite(full_name, image);

                        total_converted_pixel_count += converted_pixel_count;
                        converted_file_count += 1;
                    }
                    if (show_colors) {
                        std::cout << ", colors found:";
                        for (const auto& color : colors_found) {
                            if (color != *colors_found.begin()) {
                                std::cout << ",";
                            }
                            std::cout << " " << color_to_string(color);
                        }
                    }
                }
            }

            std::cout << std::endl;
        }

        std::cout << std::endl << "Converted a total of " << total_converted_pixel_count << " pixels in " << converted_file_count << " files" << std::endl;

        return 0;
    }
    catch (std::exception& e) {
        std::cerr << std::endl << "Error: " << e.what() << std::endl;
        std::cerr << std::endl << options.help() << std::endl;
        return 255;
    }
}
