#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace nk125 {
    class binary_file_handler {
    private:
        std::string notallowed = "*?<>|\n"; // Windows Default Disabled Chars

        std::string sanitize(std::string file_name) {
            int index = 0;
            std::string buffer = file_name;

            for (char c : buffer) {
                if (notallowed.find(c) != std::string::npos) {
                    buffer.erase(index, 1);
                }
                index++;
            }

            return buffer;
        }

        std::string read_error = "The file specified cannot be opened.";
        std::string write_error = read_error;
        // End Of Private

    public:
        typedef std::vector<unsigned char> byteArray;

        void set_not_allowed_chars(std::string chars) {
            notallowed = chars;
        }

        typedef std::vector<unsigned char> byteArray;

        template <class T = std::string>
        T read_file(std::string file_path, int flags = std::ios::in) {
            file_path = sanitize(file_path);
            std::ifstream in_file(file_path, std::ios::binary | flags);
            T m_str_buff;

            if (in_file.is_open()) {
                in_file.seekg(0, std::ios_base::end);

                std::streamoff sz = in_file.tellg();
                m_str_buff.resize(static_cast<size_t>(sz));

                in_file.seekg(0, std::ios_base::beg);
                in_file.read(reinterpret_cast<char*>(&m_str_buff[0]), sz);
                in_file.close();

                return m_str_buff;
            }
            else {
                throw std::runtime_error(read_error);
            }
        }

        std::streamoff size_file(std::string file_path) {
            file_path = sanitize(file_path);
            std::ifstream sz_file(file_path, std::ios::binary);

            if (sz_file.is_open()) {
                sz_file.seekg(0, std::ios_base::end);
                std::streamoff m_fsize = sz_file.tellg();
                sz_file.close();

                return m_fsize;
            }
            else {
                throw std::runtime_error(read_error);
            }
        }

        template <class T = std::string>
        void write_file(std::string file_path, T content, int flags = std::ios::out) {
            file_path = sanitize(file_path);
            std::ofstream out_file(file_path, std::ios::binary | flags);

            if (out_file.is_open()) {
                out_file.write(reinterpret_cast<char*>(content.data()), content.size());
                out_file.close();

                return;
            }
            else {
                throw std::runtime_error(write_error);
            }
            return;
        }

        void copy_file(std::string origin, std::string end) {
            byteArray buffer = read_file<byteArray>(origin);
            write_file<byteArray>(end, buffer);

            return;
        }
        // End of Public
    };
}
