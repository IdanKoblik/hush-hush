#pragma once

#include <string>
#include <vector>

#include "document.hpp"
#include "file_dialog.hpp"
#include "views.hpp"

struct GLFWwindow;

namespace hh {

enum class SearchTarget { File, Lsb, Dct };
enum class SearchMode { Text, Hex };

struct Search {
    bool open = false;
    bool focus = false;

    SearchTarget target = SearchTarget::File;
    SearchMode mode = SearchMode::Text;
    bool case_sensitive = false;
    char query[256] = {};

    std::vector<size_t> hits;
    size_t length = 0;
    long current = -1;
    std::string status;
};

class App {
  public:
    explicit App(GLFWwindow *window);
    ~App();

    App(const App &) = delete;
    App &operator=(const App &) = delete;

    void frame(void);
    void open(const std::string &path);

  private:
    void shortcuts(void);
    void draw_menu_bar(void);
    void draw_find_bar(void);
    void draw_streams(void);
    void draw_workspace(void);
    void draw_welcome(void);
    void draw_status_bar(void);
    void draw_modals(void);

    void upload_texture(void);
    void release_texture(void);
    void copy_selection(bool as_ascii);
    void close_document(void);

    void run_search(void);
    void step_match(int delta);
    void update_status(void);
    Highlight highlight_for(SearchTarget target) const;

    void zoom_to(float scale);

    GLFWwindow *window_ = nullptr;

    Document document_;
    Selection hex_selection_;
    Selection lsb_selection_;
    Selection dct_selection_;
    FileDialog dialog_;
    Search search_;

    std::string error_;
    unsigned int texture_ = 0;
    float menu_height_ = 0.0f;
    float ui_scale_ = 1.0f;

    float left_width_ = 300.0f;
    float right_width_ = 320.0f;

    bool show_about_ = false;
    bool show_legend_ = false;
    bool show_keys_ = false;
    bool show_goto_ = false;
    char goto_input_[32] = {};
};

} // namespace hh
