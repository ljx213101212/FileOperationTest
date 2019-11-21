#pragma once
#include "stdafx.h"

class IcoDecoder {
public :
	bool decode(unsigned char* buffer,							///< input buffer data
		int size, 										///< size of buffer
		unsigned int & width, 							///< output - width
		unsigned int & height, 							///< output - height
		std::vector<unsigned char> & image				///< output - image data
	);
};