#pragma once
#include <vector>
#include <ostream>
using namespace std;

template <typename It>
class IteratorRange{
public:
    IteratorRange(It begin, It end):
            begin_(begin),
            end_(end)
    {
    }

    auto begin() const{
        return begin_;
    }

    auto end() const{
        return end_;
    }

    auto size(){
        return distance(begin_,end_);
    }

private:
    It begin_;
    It end_;
};

template <typename Container>
class Paginator{
public:
    Paginator(Container begin, Container end, size_t page_size){
        for(auto s = begin; s != end;){
            if(s + page_size > end){
                pages_.emplace_back(s, end);
                break;
            }
            pages_.emplace_back(s, s + page_size);
            s += page_size;
        }
    }
    auto begin() const {
        return pages_.begin();
    }

    auto end() const {
        return pages_.end();
    }

    size_t size() const {
        return pages_.size();
    }
private:
    vector<IteratorRange<Container>> pages_;
};

template <typename Container>
auto Paginate(const Container& c, size_t page_size) {
    return Paginator(begin(c), end(c), page_size);
}

template <typename Iterator>
ostream& operator<<(ostream& out, const IteratorRange<Iterator>& range){
    for(auto begin = range.begin(); begin != range.end(); ++begin){
        out << *begin;
    }
    return out;
}