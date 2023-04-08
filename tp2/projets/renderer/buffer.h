#ifndef BUFFER_H
#define BUFFER_H

#include <iostream>
#include <vector>

template <typename T>
class Buffer
{
public:
    Buffer();
    Buffer(int width, int height);
    Buffer(int width, int height, const T& value);
//    Buffer(Buffer& buffer);
//    Buffer(Buffer&& buffer);
//    Buffer& operator=(const Buffer& buffer);
//    Buffer& operator=(Buffer&& buffer);

    //~Buffer();

    T* data();
    /*
     * Returns row number @index of the buffer
     */
    T* row(int index);

    void fill_values(const T& value);

    T& operator ()(int y, int x);
    const T& operator ()(int y, int x) const;

private:
    //Has the buffer been initialized ?
    bool _initialized;

    int _width, _height;

    std::vector<T> _elements;
};

template <typename T>
Buffer<T>::Buffer() :_initialized(false) {}

template <typename T>
Buffer<T>::Buffer(int width, int height) : _width(width), _height(height), _initialized(true)
{
    _elements = std::vector<T>(height * width);
}

template <typename T>
Buffer<T>::Buffer(int width, int height, const T& value) : _width(width), _height(height), _initialized(true)
{
    _initialized = true;

    _elements = std::vector<T>(width * height, value);
//    if (_elements == nullptr)
//    {
//        std::cout << "Not enough memory to allocate buffer..."    << std::endl;

//        std::exit(-1);
//    }
}

//template <typename T>
//Buffer<T>::Buffer(Buffer& buffer) : Buffer(buffer._width, buffer._height)
//{
//    _initialized = buffer._initialized;
//    _width = buffer._width;
//    _height = buffer._height;

//    if (_initialized)
//        for (int i = 0; i < buffer._height; i++)
//            for (int j = 0; j < buffer._width; j++)
//                _elements[i * _width + j] = buffer._elements[i * _width + j];
//}

//template <typename T>
//Buffer<T>::Buffer(Buffer&& buffer)
//{
//    _elements = buffer._elements;
//    buffer._elements = nullptr;

//    _initialized = buffer._initialized;
//    _width = buffer._width;
//    _height = buffer._height;
//}

//template <typename T>
//Buffer<T>& Buffer<T>::operator=(const Buffer<T>& buffer)
//{
//    _initialized = buffer._initialized;
//    _width = buffer._width;
//    _height = buffer._height;

//    if (_initialized)
//        for (int i = 0; i < buffer._height; i++)
//            for (int j = 0; j < buffer._width; j++)
//                _elements[i * _width + j] = buffer._elements[i * _width + j];

//    return *this;
//}

//template <typename T>
//Buffer<T>& Buffer<T>::operator=(Buffer<T>&& buffer)
//{
//    _elements = buffer._elements;
//    buffer._elements = nullptr;

//    _initialized = buffer._initialized;
//    buffer._initialized = false;

//    _width = buffer._width;
//    _height = buffer._height;

//    return *this;
//}

//template <typename T>
//Buffer<T>::~Buffer()
//{
//    if (_initialized)
//        delete[] _elements;
//}

template <typename T>
T* Buffer<T>::data()
{
    return _elements.data();
}

template <typename T>
T* Buffer<T>::row(int index)
{
    return &_elements[index * _width];
}

template <typename T>
void Buffer<T>::fill_values(const T& value)
{
    if (!_initialized)
        return;

    std::fill_n(_elements.begin(), _width * _height, value);
//    for (int i = 0; i < _height; i++)
//        for (int j = 0; j < _width; j++)
//            _elements[i * _width + j] = value;
}

template <typename T>
T& Buffer<T>::operator ()(int y, int x)
{
    return _elements[y * _width + x];
}

template <typename T>
const T& Buffer<T>::operator ()(int y, int x) const
{
    return _elements[y * _width + x];
}

#endif
