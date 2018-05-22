/*
 * IDer.h
 *
 *  Created on: Dec 14, 2011
 *      Author: bovi
 */
/*
Copyright (c) 2014 by Rosalba Giugno

This library contains portions of other open source products covered by separate
licenses. Please see the corresponding source files for specific terms.

RI is provided under the terms of The MIT License (MIT):

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef IDER_H_
#define IDER_H_

#include "size_t.h"

#include <map>
#include <sstream>


class IDer{
private:

	std::map<std::string, int> imap;

public:
	IDer() {
	}

	~IDer(){
	}

	s_size_t idFor(std::string* value){
		int ret = 0;
		if(value == NULL){
			ret = 0;
		}
		else{
			std::map<std::string, int>::iterator IT = imap.find(*value);
			if(IT == imap.end()){
				imap.insert(*(new std::pair<std::string, int>(*value, ((int)imap.size())+1)));
				ret = ((int)imap.size());
			}
			else{
				ret = IT->second;
			}
		}
		return ret;
	}

};


#endif /* IDER_H_ */
