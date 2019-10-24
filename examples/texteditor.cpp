#define DARWIN_FORCE_BUILTIN
#define DARWIN_DISABLE_LOG
#include "../darwin/darwin.hpp"
#include <fstream>
#include <cstdlib>
#include <vector>
#include <string>
using namespace darwin;

namespace keymap {
	constexpr int key_esc = 27;
    constexpr int key_enter = 10;
    constexpr int key_delete = 127;
	constexpr int key_up = 'w';
	constexpr int key_down = 's';
	constexpr int key_left = 'a';
	constexpr int key_right = 'd';
	constexpr int key_mode = 'i';
	constexpr int key_quit = 'q';
}

class texteditor final {
	std::size_t cursor_count = 0, cursor_x = 0, cursor_y = 0;
	std::size_t render_offx = 0, render_offy = 0;
	std::vector<std::string> file_buffer;
	darwin::drawable *pic = nullptr;
	bool insert_mode = false;
	std::string file_path;
public:
	texteditor(const std::string& path) : file_path(path)
	{
		std::ifstream in(file_path);
		for(std::string line; std::getline(in, line); file_buffer.push_back(line));
	}
	void key_up()
	{
		cursor_count = 0;
		if (cursor_y > 0)
			--cursor_y;
		else if (render_offy > 0)
			--render_offy;
	}
	void key_down()
	{
		cursor_count = 0;
		if (cursor_y < pic->get_height() - 3)
			++cursor_y;
		else if (render_offy + pic->get_height() - 2 < file_buffer.size())
			++render_offy;
		if (cursor_x + render_offx > file_buffer[cursor_y + render_offy].size()) {
			std::size_t offset = cursor_x + render_offx - file_buffer[cursor_y + render_offy].size();
			if (offset > cursor_x) {

			}
			else
				cursor_x -= offset;
		}
	}
	void key_left()
	{
		cursor_count = 0;
		if (cursor_x > 0)
			--cursor_x;
		else if (render_offx > 0)
			--render_offx;
		else {
			if (render_offy > 0)
				--render_offy;
			else if (cursor_y > 0)
				--cursor_y;
			cursor_x = file_buffer[cursor_y + render_offy].size();
		}
	}
	void key_right()
	{
		cursor_count = 0;
		if (cursor_x < pic->get_width() - 1) {
			if (cursor_x + render_offx < file_buffer[cursor_y + render_offy].size())
				++cursor_x;
			else
				key_down();
		}
		else if (cursor_x + render_offx < file_buffer[cursor_y + render_offy].size())
			++render_offx;
		else if (render_offy + pic->get_height() - 2 < file_buffer.size()) {
			render_offx = 0;
			++render_offy;
		}
	}
	void keyboard_input()
	{
		if(darwin::runtime.is_kb_hit()) {
			if (insert_mode) {
				int key = darwin::runtime.get_kb_hit();
				if (key != keymap::key_esc) {
                    auto& line = file_buffer[cursor_y + render_offy];
                    if (key == keymap::key_enter)
                    {
                        auto line_current = line.substr(0, cursor_x + render_offx);
                        auto line_next = line.substr(cursor_x + render_offx);
                        line = line_current;
                        file_buffer.insert(file_buffer.begin() + cursor_y + render_offy + 1, line_next);
                        key_down();
                        cursor_x = render_offx = 0;
                    } else if(key == keymap::key_delete) {
                        if (cursor_x + render_offx == 0) {
                            file_buffer.erase(file_buffer.begin() + cursor_y + render_offy);
                            key_up();
                            file_buffer[cursor_y + render_offy].append(line);
							
                        } else {
                            if (line.size() > 0) {
                                line.erase(cursor_x + render_offx, 1);
                                key_left();
                            }
                        }
                    } else {
                        line.insert(line.begin() + cursor_x + render_offx, key);
                        key_right();
                    }
				}
				else
					insert_mode = false;
			}
			else {
				switch (std::tolower(darwin::runtime.get_kb_hit())) {
				case keymap::key_up:
					key_up();
					break;
				case keymap::key_down:
					key_down();
					break;
				case keymap::key_left:
					key_left();
					break;
				case keymap::key_right:
					key_right();
					break;
				case keymap::key_mode:
					insert_mode = true;
					break;
				case keymap::key_quit:
					// TODO
					break;
				default:
					break;
				}
			}
		}
	}
	void render_text()
	{
		for(std::size_t y = 0; y < pic->get_height() - 2 && y + render_offy < file_buffer.size(); ++y) {
			for(std::size_t x = 0; x < pic->get_width() - 1 && x + render_offx < file_buffer[y + render_offy].size(); ++x) {
				char ch = file_buffer[y + render_offy][x + render_offx];
				pic->draw_pixel(x, y + 1, pixel(ch, true, false, colors::white, colors::black));
			}
		}
	}
	void render_cursor()
	{
		if (++cursor_count < 30) {
			darwin::runtime.get_drawable()->draw_pixel(cursor_x, cursor_y + 1, pixel(' ', true, false, colors::white, colors::white));
		}
		else if (cursor_count == 60)
			cursor_count = 0;
	}
	void run()
	{
		darwin::runtime.load("./darwin.module");
		darwin::runtime.fit_drawable();
		pic = darwin::runtime.get_drawable();
		darwin::sync_clock clock(60);
		while (true) {
			clock.reset();
			keyboard_input();
			darwin::runtime.fit_drawable();
			pic->fill(pixel(' ', true, false, colors::white, colors::black));
			pic->draw_line(0, 0, pic->get_width() - 1, 0, pixel(' ', true, false, colors::blue, colors::blue));
			pic->draw_string(2, 0, "[Darwin Texteditor]  " + file_path, pixel(' ', true, false, colors::white, colors::blue));
			pic->draw_line(0, pic->get_height() - 1, pic->get_width() - 1, pic->get_height() - 1, pixel(' ', true, false, colors::blue, colors::blue));
			if (insert_mode)
				pic->draw_string(2, pic->get_height() - 1, "INSERT (Press ESC to exit)", pixel(' ', true, false, colors::white, colors::blue));
			else
				pic->draw_string(2, pic->get_height() - 1, "W: Up S: Down A: Left D: Right I: Insert Q: Exit", pixel(' ', true, false, colors::white, colors::blue));
			render_text();
			render_cursor();
			darwin::runtime.update_drawable();
			clock.sync();
		}
	}
};

int main(int argc, char** argv)
{
	if (argc != 2)
		return -1;
	texteditor editor(argv[1]);
	editor.run();
	return 0;
}