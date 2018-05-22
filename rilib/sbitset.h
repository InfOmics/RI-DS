/*
 * sbitset.h
 *
 *  Created on: Nov 5, 2010
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

#ifndef OSBITSET_H_
#define OSBITSET_H_

#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "size_t.h"

typedef u_size_t sbitset_block;

/*
 * At least one empty block ever
 */


class sbitset{
static const size_t bytes_per_block = sizeof(sbitset_block);
static const size_t bits_per_block = sizeof(sbitset_block)*8;

public:
	sbitset_block* _bits;
	size_t _nblocks;

public:
	sbitset(){
		_bits = (sbitset_block*)calloc(1, bytes_per_block);
		_nblocks = 1;
	}

	sbitset(const sbitset& c){
		_bits = (sbitset_block*)malloc(c._nblocks * bytes_per_block);
		memcpy(_bits, c._bits, c._nblocks * bytes_per_block);
		_nblocks = c._nblocks;
	}
	sbitset(sbitset& c){
		_bits = (sbitset_block*)malloc(c._nblocks * bytes_per_block);
		memcpy(_bits, c._bits, c._nblocks * bytes_per_block);
		_nblocks = c._nblocks;
	}

	~sbitset(){
		delete [] _bits;
	}


	void printi(std::ostream& os){
		for(size_t i=0;i<_nblocks * bits_per_block;i++){
			os<<"("<<i<<":"<<get(i)<<")";
		}
	}

	void print_numbers(){
		std::cout<<"[";
		for(size_t i=0;i<_nblocks * bits_per_block;i++){
			if(get(i))
				std::cout<<i<<" ";
		}
		std::cout<<"]";
	}
	void print_info(){
	}


	bool is_empty(){
		for(size_t i = 0; i<_nblocks; i++){
			if(_bits[i]!=0){
				return false;
			}
		}
		return true;
	}

	int count_ones(){
		int count = 0;
		for(iterator IT = first_ones(); IT!=end(); IT.next_ones()){
			count++;
		}
		return count;
	}

	bool at_least_one(){
		iterator IT = first_ones();
		return IT!=end();
	}

	bool get(size_t pos){
		if(pos>=0 && pos<(_nblocks * bits_per_block)){
			return _bits[pos/bits_per_block]&(1<<(bits_per_block-(pos%bits_per_block)-1));
		}
		return 0;
	}

	void resize(size_t new_size){
		//TODO not used but nedded for future version
//		if(new_size < _nblocks * bitsof_block){
//		}
//		else
		if(new_size > _nblocks * bits_per_block){
			size_t n_nblocks = (size_t)(ceil(new_size / bits_per_block) +1);
			sbitset_block* n_bits = (sbitset_block*)calloc(n_nblocks, bytes_per_block);
			memcpy(n_bits, _bits, _nblocks * bytes_per_block);
			_nblocks = n_nblocks;
			delete [] _bits;
			_bits = n_bits;
		}
	}

	void resizeAsis(size_t new_size){
		//TODO not used but nedded for future version
//		if(new_size < _nblocks * bitsof_block){
//		}
//		else
		if(new_size > _nblocks * bits_per_block){
			size_t n_nblocks = (size_t)(ceil(new_size / bits_per_block) +1);
			delete [] _bits;
			_bits = (sbitset_block*)malloc(n_nblocks * bytes_per_block);
			_nblocks = n_nblocks;
		}
	}



	void set(size_t pos, bool value){
		if(pos>=0){
			if(pos >= _nblocks * bits_per_block){
				resize(pos+1);
			}
			if(value!=get(pos))
				this->_bits[pos/bits_per_block] ^= 1<<(bits_per_block-(pos%bits_per_block)-1);
		}
	}


	sbitset& operator&=(sbitset& bs){
		size_t wblokcs = _nblocks <= bs._nblocks ? _nblocks : bs._nblocks;
		for(size_t i = 0; i<wblokcs; i++)
			_bits[i] &= bs._bits[i];
		for(size_t i = wblokcs; i< _nblocks; i++)
			_bits[i] = 0;
		return *this;
	}

	sbitset& operator|=(sbitset& bs){
		size_t wblokcs = _nblocks <= bs._nblocks ? _nblocks : bs._nblocks;
		for(size_t i = 0; i<wblokcs; i++)
			_bits[i] |= bs._bits[i];
		if(bs._nblocks > _nblocks){
			resize(bs._nblocks * bits_per_block);
			for(size_t i = wblokcs; i<_nblocks; i++)
				_bits[i] = bs._bits[i];
		}
		return *this;
	}

	sbitset& operator =(sbitset& b){
		delete [] _bits;
		_nblocks = b._nblocks;
		_bits = (sbitset_block*)malloc(_nblocks * bytes_per_block);
		memcpy(_bits, b._bits, _nblocks * bytes_per_block);
		return *this;
	}

	bool operator!=(sbitset& b){
		size_t wblokcs = _nblocks <= b._nblocks ? _nblocks : b._nblocks;
		for(size_t i = 0; i<wblokcs; i++){
			if(_bits[i] != b._bits[i]){
				return true;
			}
		}
		if(wblokcs > _nblocks){
			for(size_t i = wblokcs; i<b._nblocks; i++){
				if(b._bits[i] != 0){
					return true;
				}
			}
		}
		else{
			for(size_t i = wblokcs; i<_nblocks; i++){
				if(_bits[i] != 0){
					return true;
				}
			}
		}
		return false;
	}


	bool emptyAND(sbitset& b){
		size_t wblokcs = _nblocks <= b._nblocks ? _nblocks : b._nblocks;
		for(size_t i = 0; i<wblokcs; i++){
			if((_bits[i] & b._bits[i]) != 0){
				return false;
			}
		}
		return true;
	}


	void setAll(size_t size, bool value){
		resizeAsis(size);
		sbitset_block svalue = 0x0;
		sbitset_block mask = value;
		for(size_t i=0; i<bits_per_block; i++){
			svalue |= mask;
			mask = mask << 1;
		}
		for(size_t i=0; i<_nblocks; i++){
			_bits[i] = svalue;
		}
	}


	/* ============================================================
	 * Iterators
	 * ============================================================
	 */
	class iterator{
		sbitset& _sb;
		u_size_t _wblock;
		unsigned short _shift;
		public:
			u_size_t first;	//position
			bool second;	//value
		public:
			iterator(sbitset &sb) : _sb(sb){
				_wblock = 0;
				_shift = bits_per_block - 1;
				get_comps();
			}
			iterator(sbitset &sb, u_size_t wblock) : _sb(sb), _wblock(wblock){
				_shift = bits_per_block - 1;
				get_comps();
			}

		private:
			void get_comps(){
				first = (_wblock * bits_per_block) + bits_per_block - _shift - 1;
				if(_wblock<_sb._nblocks){
					second =  _sb._bits[_wblock] & 1<<(_shift);
				}
				else{
					second = false;
				}
			}
		public:
			iterator& operator++(){
				if(_shift == 0){
					_wblock++;
					_shift = bits_per_block - 1;
				}
				else{
					_shift--;
				}

				get_comps();
				return (*this);
			}
			void operator++ (int){
			   ++(*this);
			}
			void next_ones(){
				do{
					if(_shift == 0){
						_wblock++;
						_shift = bits_per_block - 1;
						while(_wblock<_sb._nblocks && _sb._bits[_wblock]==0)
							_wblock++;
					}
					else{
						_shift--;
					}

					get_comps();
				}while(!second && _wblock<_sb._nblocks);
			}


			const bool operator==(const iterator& IT){
				return &_sb == &(IT._sb) && _wblock == IT._wblock &&  _shift == IT._shift;
			}
			const bool operator!=(const iterator& IT){
				if( &_sb != &(IT._sb) || _wblock != IT._wblock ||  _shift != IT._shift)
					return true;
				return false;
			}
	};
//	iterator& begin(){
//		if(is_empty())
//			return end();
//		iterator *IT = new iterator(*this);
//		return *IT;
//	}
//	iterator& first_ones(){
//		if(is_empty())
//			return end();
//		iterator *IT = new iterator(*this);
//		if(IT->second != 1)
//			IT->next_ones();
//		return *IT;
//	}
//	iterator& end(){
//		iterator *IT = new iterator(*this, _nblocks);
//		return *IT;
//	}
	iterator begin(){
		if(is_empty())
			return end();
		return iterator(*this);
	}

	iterator end(){
		return iterator(*this, _nblocks);
	}

	iterator first_ones(){
		if(is_empty())
			return end();
		iterator IT(*this);
		if(IT.second != 1)
			IT.next_ones();
		return IT;
	}
};

#endif /* SBITSET_H_ */
