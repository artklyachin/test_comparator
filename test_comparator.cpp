#include <iostream>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <regex>
#include <algorithm>
#include <limits>
#include <cmath>


namespace fs = std::filesystem;
constexpr double NaN = std::numeric_limits<double>::quiet_NaN();


fs::path shortcut(fs::path global_path, fs::path cut_path)
{
    std::string res = global_path.string().substr(cut_path.string().size() + 1, global_path.string().size());
    return res;
}


std::vector<fs::path> paths_to(const fs::path& path_to)
{
    std::vector<fs::path> vec_paths;
    for (const auto& entry1 : fs::recursive_directory_iterator(path_to)) {
        if (entry1.path().extension() == ".stdout") {
            vec_paths.push_back(shortcut(entry1.path(), path_to).string());
        }
    }
    return vec_paths;
}



std::vector<fs::path> compare_lists(std::vector<fs::path>& A, std::vector<fs::path>& B)
{
    std::vector<fs::path> in_A_not_in_B;
    for (fs::path i : A) {
        bool exist = false;
        for (fs::path j : B) {
            if (i == j) exist = true;
        }
        if (!exist) {
            in_A_not_in_B.push_back(i);
        }
    }
    return in_A_not_in_B;
}


bool test1(const fs::path& path_to_run, const fs::path& path_to_ref, std::stringstream& ss)
{
    bool fail = false;
    if (!fs::is_directory(fs::status(path_to_run))) {
        ss << "directory missing: ft_run" << std::endl;
        fail = true;
    }
    if (!fs::is_directory(fs::status(path_to_ref))) {
        ss << "directory missing: ft_reference" << std::endl;
        fail = true;
    }
    return fail;
}

bool test2(const fs::path& path_to_run, const fs::path& path_to_ref, std::stringstream& ss)
{
    bool fail = false;
    //список путей
    std::vector<fs::path> vec_paths_to_run = paths_to(path_to_run);
    std::vector<fs::path> vec_paths_to_ref = paths_to(path_to_ref);
    //сравнение списков путей
    std::vector<fs::path> in_run_not_in_ref = compare_lists(vec_paths_to_run, vec_paths_to_ref);
    std::vector<fs::path> in_ref_not_in_run = compare_lists(vec_paths_to_ref, vec_paths_to_run);
    fail = in_run_not_in_ref.size() != 0 || in_ref_not_in_run.size() != 0;

    if (fail) {
        if (in_ref_not_in_run.size() > 0) {
            ss << "In ft_run there are missing files present in ft_reference: " << "\'" << in_ref_not_in_run[0].string() << "\'";
            for (int i = 1; i < int(in_ref_not_in_run.size()); ++i) {
                ss << ", " << "\'" << in_ref_not_in_run[i].string() << "\'";
            }
            ss << std::endl;
        }
        if (in_run_not_in_ref.size() > 0) {
            ss << "In ft_run there are extra files not present in ft_reference: " << "\'" << in_run_not_in_ref[0].string() << "\'";
            for (int i = 1; i < int(in_run_not_in_ref.size()); ++i) {
                ss << ", " << "\'" << in_run_not_in_ref[i].string() << "\'";
            }
            ss << std::endl;
        }
    }
    return fail;
}

bool test3(const fs::path& path_to_run, fs::path path, std::stringstream& ss)
{
    std::regex regex_error("\\berror\\b", std::regex::icase);
    std::regex regex_finished("^solver finished at", std::regex::icase);
    bool fail = false;

    //run
    std::ifstream ifs_run(path_to_run / path);
    std::string ifs_line;
    int num_of_line = 1;
    bool find_finish_word = false;
    while (getline(ifs_run, ifs_line)) {
        if (std::regex_search(ifs_line, regex_error)) {
            ss << path.string() << "(" << num_of_line << "): " << ifs_line << std::endl;
            fail = true;
        }
        if (std::regex_search(ifs_line, regex_finished)) {
            find_finish_word = true;
        }
        ++num_of_line;
    }
    if (!find_finish_word) {
        ss << path.string() << ": missing \'Solver finished at\'" << std::endl;
        fail = true;
    }
    return fail;
}

bool test4(const fs::path& path_to_run, const fs::path& path_to_ref, fs::path path, std::stringstream& ss)
{
    //Memory Working Set Current = 260.2 Mb, Memory Working Set Peak = 724.2 Mb
    std::regex regex_memory_working("^Memory Working Set Current = \\d+\\.?\\d* Mb, Memory Working Set Peak = (\\d+\\.?\\d*) Mb");
    //MESH::Bricks: Total=1772 Gas=592 Solid=456 Partial=724 Irregular=0
    std::regex regex_bricks("^MESH::Bricks: Total=(\\d+\\.?\\d*) Gas=\\d+\\.?\\d* Solid=\\d+\\.?\\d* Partial=\\d+\\.?\\d* Irregular=\\d+\\.?\\d*");
    bool fail = false;

    //run
    std::ifstream ifs_run(path_to_run / path);
    std::string ifs_line;
    double max_memory_peak_run = NaN, last_total_run = NaN;
    while (getline(ifs_run, ifs_line)) {

        std::smatch match3;
        if (std::regex_match(ifs_line, match3, regex_memory_working)) {
            double x = stod(match3[1].str());
            if (isnan(max_memory_peak_run) || max_memory_peak_run < x) {
                max_memory_peak_run = x;
            }
        }
        std::smatch match4;
        if (std::regex_search(ifs_line, match4, regex_bricks)) {
            double x = stod(match4[1].str());
            last_total_run = x;
        }
    }

    //ref
    std::ifstream ifs_ref(path_to_ref / path);
    double max_memory_peak_ref = NaN, last_total_ref = NaN;
    while (getline(ifs_ref, ifs_line)) {
        std::smatch match3;
        if (std::regex_match(ifs_line, match3, regex_memory_working)) {
            double x = stod(match3[1].str());
            if (isnan(max_memory_peak_ref) || max_memory_peak_ref < x) {
                max_memory_peak_ref = x;
            }
        }
        std::smatch match4;
        if (std::regex_search(ifs_line, match4, regex_bricks)) {
            double x = stod(match4[1].str());
            last_total_ref = x;
        }
    }
    if (!isnan(max_memory_peak_ref) && !isnan(max_memory_peak_run)) {
        double real_diff = (max_memory_peak_run - max_memory_peak_ref) / max_memory_peak_ref;
        double criterion = 0.5;
        if (abs(real_diff) > criterion) {
            ss << path.string() << ": different \'Memory Working Set Peak\' (ft_run=" << max_memory_peak_run << ", ft_reference=" << max_memory_peak_ref << ", rel.diff=" << std::setprecision(2) << real_diff << std::setprecision(6) << ", criterion=" << criterion << ")" << std::endl;
            fail = true;
        }
    }
    if (!isnan(last_total_ref) && !isnan(last_total_run)) {
        double real_diff = (last_total_run - last_total_ref) / last_total_ref;
        double criterion = 0.1;
        if (abs(real_diff) > criterion) {
            ss << path.string() << ": different \'Total\' of bricks (ft_run=" << last_total_run << ", ft_reference=" << last_total_ref << ", rel.diff=" << std::setprecision(2) << real_diff << std::setprecision(6) << ", criterion=" << criterion << ")" << std::endl;
            fail = true;
        }
    }
    return fail;
}

void print_fail(std::stringstream& ss, std::ofstream& ofs, const fs::path& path_to_test, const fs::path& path_to_log)
{
    std::cout << "FAIL: " << shortcut(path_to_test, path_to_log).string() << std::endl;
    std::cout << ss.str();
    ofs << ss.str();
    ofs << "FAIL" << std::endl;
}

void print_ok(std::ofstream& ofs, const fs::path& path_to_test, const fs::path& path_to_log)
{
    std::cout << "OK: " << shortcut(path_to_test, path_to_log).string() << std::endl;
    ofs << "OK" << std::endl;
}


void tests(fs::path path_to_log)
{
    for (const auto& global_entry : fs::directory_iterator(path_to_log)) {
        for (const auto& entry : fs::directory_iterator(global_entry.path())) {

            std::ofstream ofs(entry.path() / "report.txt");
            const auto path_to_ref = entry.path() / "ft_reference";
            const auto path_to_run = entry.path() / "ft_run";
            std::stringstream ss;
            bool fail;
            //1
            fail = test1(path_to_run, path_to_ref, ss);
            if (fail) {
                print_fail(ss, ofs, entry.path(), path_to_log);
                continue;
            }

            //2
            fail = test2(path_to_run, path_to_ref, ss);
            if (fail) {
                print_fail(ss, ofs, entry.path(), path_to_log);
                continue;
            }

            //3,4
            std::vector<fs::path> vec_paths_to_run = paths_to(path_to_run);

            for (fs::path path : vec_paths_to_run) {
                bool fail3 = test3(path_to_run, path, ss);
                bool fail4 = test4(path_to_run, path_to_ref, path, ss);
                fail = fail || fail3 || fail4;
            }
            if (fail) {
                print_fail(ss, ofs, entry.path(), path_to_log);
                continue;
            }

            print_ok(ofs, entry.path(), path_to_log);
        }
    }
    return;
}

int main(int argc, char** argv)
{
    if (argc != 2) {
        std::cerr << "Usage:" << argv[0] << " path\n";
        return 1;
    }
    std::string path = argv[1];
    
    //std::string path = "D:\\art\\Programming\\internship\\mentor\\Task\\logs";
    tests(path);
}
