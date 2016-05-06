#pragma once

#include <string>

template <typename T>
struct Buffer {
private:
	T * _buf;
	int _size;
	int _len;
public:
	Buffer() : _buf(NULL), _size(0), _len(0) {}
	~Buffer() {
		clear();
	}
	int length() const { return _len; }
	int size() const { return _size; }
	void reset() {
		_len = 0;
		if (_buf)
			_buf[_len] = 0;
	}
	void clear() {
		_len = 0;
		_size = 0;
		if (_buf) {
			free(_buf);
			_buf = NULL;
		}
	}
	void reserve(int sz) {
		// ensure there is sz free items in buffer
		int newSize = _len + sz + 1;
		if (newSize > _size) {
			if (newSize < 64) {
				newSize = 64;
			} else {
				if (newSize < _size * 2)
					newSize = _size * 2;
			}
			_buf = (T*)realloc(_buf, newSize * sizeof(T));
			for (int i = _size; i < newSize; i++)
				_buf[i] = 0; // fill with zeroes
			_size = newSize;
		}
	}
	/// if length is greater than requested, remove extra items from end of buffer
	void truncate(int newLength) {
		if (newLength >= 0 && _len > newLength) {
			_len = newLength;
			if (_buf)
				_buf[_len] = 0;
		}
	}
	void append(T item) {
		reserve(1);
		_buf[_len++] = item;
		_buf[_len] = 0; // trailing zero, just in case
	}
	Buffer & operator += (T item) {
		append(item);
		return *this;
	}
	Buffer & operator += (const Buffer & items) {
		append(items.c_str(), items.length());
		return *this;
	}
	/// append z-string
	Buffer & operator += (const T * items) {
		append(items);
		return *this;
	}
	/// append several items
	void append(const T * items, size_t count) {
		if (count <= 0 || !items)
			return;
		reserve(count);
		for (unsigned i = 0; i < count; i++) {
			_buf[_len + i] = items[i];
		}
		_len += count;
		_buf[_len] = 0; // trailing zero, just in case
	}
	void append(const T * s) {
		int len = 0;
		for (; s[len]; len++)
			;
		append(s, len);
	}
	void assign(const T * s, size_t count) {
		reset();
		append(s, count);
	}
	void assign(const T * s) {
		reset();
		append(s);
	}
	/// return item by index, or 0 if index out of bounds
	T operator[] (int index) { return (index >= 0 && index < _len) ? _buf[index] : 0; }
	const T * c_str() const {
		return _buf;
	}
	T * ptr() {
		return _buf;
	}
	bool empty() const { return _length == 0; }
	bool isNull() const { return !_buf; }
	/// returns last item
	T last() const { return (_len > 0) ? _buf[_len - 1] : 0; }
	/// return true if last item equals to specified one
	bool endsWith(T item) const {
		return (last() == item);
	}
	void trimEol() {
		while (last() == '\n' || last() == '\r')
			truncate(_len - 1);
	}
};

std::string toUtf8(const std::wstring s);
std::wstring toUtf16(const std::string s);

struct StringBuffer : public Buffer<char> {
	StringBuffer & operator = (const std::string & s) { assign(s.c_str(), s.length()); return *this; }
	std::string str() { return std::string(c_str(), length()); }
	std::wstring wstr() { return toUtf16(str()); }
};

struct WstringBuffer : public Buffer<wchar_t> {
	WstringBuffer & operator = (const std::wstring & s) { assign(s.c_str(), s.length()); return *this; }
	std::string str() { return toUtf8(wstr()); }
	std::wstring wstr() { return std::wstring(c_str(), length()); }
};

