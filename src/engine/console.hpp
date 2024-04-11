#pragma once
#include <cstdlib>
#include <cstdint>
#include <iostream>
#include <iomanip>
#include <list>
#include <utility>

#include <boost/predef.h>

#if defined(BOOST_OS_WINDOWS)
#include <windows.h>
#endif

namespace ms_notify{

class Console{
	enum eMask{
		bgMask = BACKGROUND_BLUE | BACKGROUND_GREEN | BACKGROUND_RED | BACKGROUND_INTENSITY,
		fgMask = FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_INTENSITY
	};

	enum eForeGroundColour{
		fgBlack = 0,
		fgLoRed = FOREGROUND_RED,
		fgLoGreen = FOREGROUND_GREEN,
		fgLoBlue = FOREGROUND_BLUE,
		fgLoCyan = fgLoGreen | fgLoBlue,
		fgLoMagenta = fgLoRed | fgLoBlue,
		fgLoYellow = fgLoRed | fgLoGreen,
		fgLoWhite = fgLoRed | fgLoGreen | fgLoBlue,
		fgGray = fgBlack | FOREGROUND_INTENSITY,
		fgHiWhite = fgLoWhite | FOREGROUND_INTENSITY,
		fgHiBlue = fgLoBlue | FOREGROUND_INTENSITY,
		fgHiGreen = fgLoGreen | FOREGROUND_INTENSITY,
		fgHiRed = fgLoRed | FOREGROUND_INTENSITY,
		fgHiCyan = fgLoCyan | FOREGROUND_INTENSITY,
		fgHiMagenta = fgLoMagenta | FOREGROUND_INTENSITY,
		fgHiYellow = fgLoYellow | FOREGROUND_INTENSITY
	};

	enum eBackGroundColour{
		bgBlack = 0,
		bgLoRed = BACKGROUND_RED,
		bgLoGreen = BACKGROUND_GREEN,
		bgLoBlue = BACKGROUND_BLUE,
		bgLoCyan = bgLoGreen | bgLoBlue,
		bgLoMagenta = bgLoRed | bgLoBlue,
		bgLoYellow = bgLoRed | bgLoGreen,
		bgLoWhite = bgLoRed | bgLoGreen | bgLoBlue, 
		bgGray = bgBlack | BACKGROUND_INTENSITY, 
		bgHiWhite = bgLoWhite | BACKGROUND_INTENSITY, 
		bgHiBlue = bgLoBlue | BACKGROUND_INTENSITY, 
		bgHiGreen = bgLoGreen | BACKGROUND_INTENSITY, 
		bgHiRed = bgLoRed | BACKGROUND_INTENSITY, 
		bgHiCyan = bgLoCyan | BACKGROUND_INTENSITY, 
		bgHiMagenta = bgLoMagenta | BACKGROUND_INTENSITY, 
		bgHiYellow = bgLoYellow | BACKGROUND_INTENSITY
	};

public:
    inline Console()
	: hCon_(GetStdHandle(STD_OUTPUT_HANDLE))
	{ }

	inline Console& clr(){
		colors_.push_back(std::make_pair(-1, -1));
		return *this;
	}

	inline Console& fg_green(){
		colors_.push_back(std::make_pair(fgHiGreen, bgMask));
		return *this;
	}

	inline Console& fg_red(){
		colors_.push_back(std::make_pair(fgHiRed, bgMask));
		return *this;
	}

	inline Console& fg_white(){
		colors_.push_back(std::make_pair(fgHiWhite, bgMask));
		return *this;
	}

	inline Console& fg_blue(){
		colors_.push_back(std::make_pair(fgHiBlue, bgMask));
		return *this;
	}

	inline Console& fg_cyan(){
		colors_.push_back(std::make_pair(fgHiCyan, bgMask));
		return *this;
	}

	inline Console& fg_magenta(){
		colors_.push_back(std::make_pair(fgHiMagenta, bgMask));
		return *this;
	}

	inline Console& fg_yellow(){
		colors_.push_back(std::make_pair(fgHiYellow, bgMask));
		return *this;
	}

	inline Console& fg_black(){
		colors_.push_back(std::make_pair(fgBlack, bgMask));
		return *this;
	}
	
	inline Console& fg_gray(){
		colors_.push_back(std::make_pair(fgGray, bgMask));
		return *this;
	}

	inline Console& fg_default(){
		colors_.push_back(std::make_pair(fgLoWhite, bgMask));
		return *this;
	}

protected:
    inline void Clear(){
        COORD coordScreen = { 0, 0 };
        
        GetInfo(); 
        FillConsoleOutputCharacter(hCon_, ' ', dwConSize_, coordScreen, &cCharsWritten_); 

        GetInfo(); 
        FillConsoleOutputAttribute(hCon_, csbi_.wAttributes, dwConSize_, coordScreen, &cCharsWritten_); 
        SetConsoleCursorPosition(hCon_, coordScreen); 
    }

    inline void SetColor(WORD wRGBI, WORD Mask){
        GetInfo();
        csbi_.wAttributes &= Mask; 
        csbi_.wAttributes |= wRGBI; 
        SetConsoleTextAttribute(hCon_, csbi_.wAttributes);
    }

private:
	friend std::ostream& operator<< (std::ostream& os, Console& /*c*/);

	inline void Flush(){
		if(!colors_.empty()){
			if(colors_.back().first == -1) Clear();
			else SetColor(colors_.back().first, colors_.back().second);

			colors_.pop_back();
		}
	}

    void GetInfo(){
        GetConsoleScreenBufferInfo(hCon_, &csbi_);
        dwConSize_ = csbi_.dwSize.X * csbi_.dwSize.Y; 
    }

	HANDLE hCon_;
	DWORD cCharsWritten_; 
	CONSOLE_SCREEN_BUFFER_INFO csbi_; 
	DWORD dwConSize_;

	std::list< std::pair< int, int > > colors_;
};
    
inline std::ostream& operator<< (std::ostream& os, Console& c){
    os.flush();
	c.Flush();
	return os;
}

} //ms_notify
