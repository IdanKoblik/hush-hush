#pragma once

#include <string>
#include <vector>

namespace hh {

class FileDialog {
  public:
    void open(const std::string &start_directory);

    bool draw(std::string &chosen);

  private:
    struct Entry {
        std::string name;
        bool directory = false;
    };

    void navigate(const std::string &directory);

    bool pending_ = false;
    std::string directory_;
    std::string error_;
    std::vector<Entry> entries_;
    int selected_ = -1;
    char input_[1024] = {};
};

} // namespace hh
