//ringbuffer.h
#pragma once

#include <vector>

/*
 * The Ringbuffer holds a vector, and allows range-based for loops to go from
 * the "front_index", to the end, then start from the beginning and continuing
 * iterating to the "front_index".  It offeres limited functionality as a
 * general container, but is useful for the pendulums.
 *
 * example:
 * RingBuffer<int> rbi; // only default construction
 * rbi.Fill(3,0); // resizes the buffer to "3", fills all positions with 0
 * for (int i = 0; i < 5; ++i) rbi.Push(i);
 *          // Push(val) puts "val" into the position after "front_index", and
 *          // wraps around to the beginning once the end is reached, rbi.buffer
 *          // now holds {3,4,5}
 * for (auto& i : rbi) rbi *= 2; //rbi.buffer == {6,8,10}
 *
 * rbi.Translate(2); 
 *          // Translate() uses += to add a value to every item. rbi.buffer ==
 *          // {8, 10, 12}
 *
 *
 */

using std::vector;

template<typename T>
class RingBufferIterator {
  public:

    size_t     pos;
    vector<T>* underlyingVector;

    /*
     * size_t& operator++() 
     */
    size_t& operator++()
    { 
      ++pos %= underlyingVector->size();
      return pos;
    }
    /*
     * size_t& operator--()
     */
    size_t& operator--()
    {
      (pos == 0) ? pos = underlyingVector->size() - 1 : --pos;
      return pos;
    }
    /*
     * size_t operator++(int)
     */
    size_t operator++(int)
    {
      auto tmp = *this;
      this->operator++();
      return tmp;
    }
    /*
     * size_t operator--(int)
     */
    size_t operator--(int)
    {
      auto tmp = *this;
      this->operator--();
      return tmp;
    }
    T& operator*()              { return (*underlyingVector)[pos]; }
    T* operator->()             { return &(*underlyingVector)[pos]; }
    const T& operator*() const  { return (*underlyingVector)[pos]; }
    const T* operator->() const { return &(*underlyingVector)[pos]; }
    bool operator==(const RingBufferIterator& l) const { return pos == l.pos; }
    bool operator!=(const RingBufferIterator& l) const { return !(*this == l); }

};

/*
 * The type used for the ringbuffer must be support default construction, copy
 * construction, and (if Translate is to be used) +=,
 */
template<typename T>
class RingBuffer {
  public:
    using Iterator = RingBufferIterator<T>;

    RingBuffer() : front_index(0), buffer() {}

    /*
     * begin
     */
    Iterator begin()
    {
      if (front_index == buffer.size() - 1) return Iterator{0, &buffer};
      else return {front_index + 1, &buffer};
    }
    /*
     * end
     */
    Iterator end()
    {
      return {front_index, &buffer};
    }
    /*
     * front
     */
    T& front()
    {
      return *begin();
    }
    /*
     * Push
     */
    void Push(const T& position)
    {
      if (buffer.size() == 0)
      {
        buffer.push_back(position);
      } else {
        ++front_index %= buffer.size();
        buffer[front_index] = position; 
      }
    }
    /*
     * Fill
     */
    void Fill(size_t size, const T& pos)
    {
      buffer.clear();
      buffer.reserve(size);
      for(size_t i = 0; i < size; ++i) buffer.push_back(pos);
    }
    /*
     * Translate
     */
    void Translate(const T& p)
    {
      for (auto& pos : buffer) { pos += p; }
    }

          T& operator[](size_t i)       { return buffer[i]; }
    const T& operator[](size_t i) const { return buffer[i]; }

    //TODO make these private, possibly needing a friend class...
    size_t front_index;
    vector<T> buffer;
};

