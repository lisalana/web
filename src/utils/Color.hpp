#ifndef COLOR_HPP
#define COLOR_HPP

#include <iostream>
#include <sstream>
#include <string>

class Color {
private:
    std::string _color;
    
    std::string _int_to_string(long value) {
        std::ostringstream oss;
        oss << value;
        return oss.str();
    }

public:
    // RGB
    Color(unsigned char r, unsigned char g, unsigned char b) {
        this->_color = "\033[38;2;" + this->_int_to_string(r) + ";" 
                      + this->_int_to_string(g) + ";" + this->_int_to_string(b) + "m";
    }
    
    // Hex
    Color(unsigned int hex) {
        unsigned char r = (hex >> 16) & 0xFF;
        unsigned char g = (hex >> 8) & 0xFF;
        unsigned char b = hex & 0xFF;
        this->_color = "\033[38;2;" + this->_int_to_string(r) + ";" 
                      + this->_int_to_string(g) + ";" + this->_int_to_string(b) + "m";
    }
    
    // Background color
    Color(unsigned char r, unsigned char g, unsigned char b, bool background) {
        std::string code = background ? "48" : "38";  // 48 for background, 38 for foreground
        this->_color = "\033[" + code + ";2;" + this->_int_to_string(r) + ";" 
                      + this->_int_to_string(g) + ";" + this->_int_to_string(b) + "m";
    }
    
    std::string get() const {
        return this->_color;
    }

    friend std::ostream& operator<<(std::ostream& oss, const Color& color);
};

// Predefined colors namespace
namespace Colors {
    static const Color RED(255, 0, 0);
    static const Color GREEN(0, 255, 0);
    static const Color BLUE(0, 0, 255);
    static const Color YELLOW(255, 255, 0);
    static const Color MAGENTA(255, 0, 255);
    static const Color CYAN(0, 255, 255);
    static const Color WHITE(255, 255, 255);
    static const Color BLACK(0, 0, 0);
    static const Color ORANGE(0xFF5733);
    static const Color PURPLE(0x800080);
    static const Color GRAY(128, 128, 128);
    static const Color LIGHT_GREEN(144, 238, 144);
    static const Color LIGHT_BLUE(173, 216, 230);
    static const Color DARK_RED(139, 0, 0);
}

// Text format namespace
namespace TextFormat {
    static const std::string BOLD = "\033[1m";
    static const std::string ITALIC = "\033[3m";
    static const std::string UNDERLINE = "\033[4m";
    static const std::string DIM = "\033[2m";
    static const std::string STRIKETHROUGH = "\033[9m";
    static const std::string RESET = "\033[0m";
    static const std::string BLINK = "\033[5m";
    static const std::string REVERSE = "\033[7m";
}

// Background colors namespace
namespace BgColors {
    static const Color RED(255, 0, 0, true);
    static const Color GREEN(0, 255, 0, true);
    static const Color BLUE(0, 0, 255, true);
    static const Color YELLOW(255, 255, 0, true);
    static const Color MAGENTA(255, 0, 255, true);
    static const Color CYAN(0, 255, 255, true);
    static const Color WHITE(255, 255, 255, true);
    static const Color BLACK(0, 0, 0, true);
}

inline std::ostream& operator<<(std::ostream& oss, const Color& color) {
    oss << color.get();
    return oss;
}

#endif