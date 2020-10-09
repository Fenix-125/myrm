// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

// Remember to include ALL the necessary headers!
#include <iostream>
#include <boost/program_options.hpp>

#include <sys/stat.h>
#include <unistd.h>
// dir traversal
#include <sys/types.h>
#include <dirent.h>

enum class confirm_answers {
    YES, NO, ALL, CANCEL, INVALID
};

static inline bool dir_exists(const std::string &path);

static inline confirm_answers get_confirm_answer(const std::string &path);

static bool rm_dir_recursive(const std::string &path);

int main(int argc, char **argv) {
    std::vector<std::string> files{};
    bool force, recursive;

    namespace po = boost::program_options;

    po::options_description visible("Supported options");
    visible.add_options()
            ("help,h", "Print this help message.")
            ("force,f", po::value<bool>(&force)->zero_tokens(), "Delete without acceptance")
            ("recursive,R", po::value<bool>(&recursive)->zero_tokens(),
             "Remove directories and their contents recursively");

    po::options_description hidden("Hidden options");
    hidden.add_options()
            ("f_names", po::value<std::vector<std::string>>(&files)->composing(),
             "file path's to be deleted.");

    po::positional_options_description p;
    p.add("f_names", -1);

    po::options_description all("All options");
    all.add(visible).add(hidden);

    po::variables_map vm;
    try {
        po::store(po::command_line_parser(argc, argv).options(all).positional(p).run(), vm);
    } catch (const boost::wrapexcept<boost::program_options::unknown_option> &e) {
        std::cerr << "Error: unknown option '" << e.get_option_name() << "'" << std::endl;
        return EXIT_FAILURE;
    }

    po::notify(vm);

    if (vm.count("help")) {
        std::cout << "Usage:\n  myrm [-h|--help] [-f] [-R] <file1> <file2> <file3>\n" << visible << std::endl;
        return EXIT_SUCCESS;
    }
    if (files.empty()) {
        std::cerr << "Error: no file path's specified!" << std::endl;
        return EXIT_FAILURE;
    }

    struct stat status{};
    for (auto &path : files) {
        if (!force) {
            switch (get_confirm_answer(path)) {
                case confirm_answers::YES:
                    break;
                case confirm_answers::NO:
                    continue;
                case confirm_answers::ALL:
                    force = true;
                    break;
                case confirm_answers::CANCEL:
                    return EXIT_SUCCESS;
                case confirm_answers::INVALID:
                    std::cerr << "Error: invalid confirm input!" << std::endl;
                    return EXIT_FAILURE;
            }
        }
        if (stat(path.data(), &status) != 0) {
            std::cerr << "Error: file (" << path << ") do not exists!" << std::endl;
            return EXIT_FAILURE;
        }
        if (S_ISDIR(status.st_mode)) {
            if (recursive) {
                if (!rm_dir_recursive(path)) {
                    return EXIT_FAILURE;
                }
            } else {
                std::cerr << "Error: file (" << path << ") is a dir! Can not remove a dir!" << std::endl;
                return EXIT_FAILURE;
            }
        } else {
            if (unlink(path.data()) != 0) {
                std::cerr << "Error: can not remove file " << path << std::endl;
                return EXIT_FAILURE;
            }
        }
    }
    return EXIT_SUCCESS;
}

static bool rm_dir_recursive(const std::string &path) {
    std::string tmp_path = path;
    if (path[path.size() - 1] != '/')
        tmp_path += '/';
    const auto loop_back = tmp_path.rfind("./");
    const auto prev_dir_loop = tmp_path.rfind("../");
    if ((loop_back == tmp_path.size() - 2 && loop_back != std::string::npos) ||
        (prev_dir_loop == tmp_path.size() - 3 && prev_dir_loop != std::string::npos))
        return true;
    DIR *dir_st = opendir(tmp_path.data());
    if (dir_st == nullptr) {
        std::cerr << "Error: can not open directory " << path << std::endl;
        return false;
    }
    struct dirent *dir_entry;

    while ((dir_entry = readdir(dir_st)) != nullptr) {
        switch (dir_entry->d_type) {
            case DT_DIR:
                if (!rm_dir_recursive(tmp_path + std::string{dir_entry->d_name})) {
                    return false;
                }
                break;
            default:
                if (unlink((tmp_path + std::string{dir_entry->d_name}).data()) != 0) {
                    std::cerr << "Error: failed to delete entry: "
                              << tmp_path + std::string{dir_entry->d_name} << std::endl;
                    return false;
                }
                break;
        }
    }
    if (rmdir(path.data()) != 0) {
        std::cerr << "Error: failed to delete directory entry: " << path << std::endl;
        return false;
    }
    return true;
}

static inline confirm_answers get_confirm_answer(const std::string &path) {
    std::string answer;
    std::cout << "Delete file '" << path << "' type Y[es]/N[o]/A[ll]/C[ancel]: " << std::flush;
    getline(std::cin, answer);

    if (answer.empty())
        return confirm_answers::NO;
    std::transform(answer.begin(), answer.end(), answer.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    if (answer == "y" || answer == "yes") {
        return confirm_answers::YES;
    } else if (answer == "n" || answer == "no") {
        return confirm_answers::NO;
    } else if (answer == "a" || answer == "all") {
        return confirm_answers::ALL;
    } else if (answer == "c" || answer == "cancel") {
        return confirm_answers::CANCEL;
    } else {
        return confirm_answers::INVALID;
    }
}
