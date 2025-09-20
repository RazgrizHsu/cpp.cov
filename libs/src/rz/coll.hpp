#pragma once

#include <set>

namespace rz::coll {

template<typename T>
typename std::vector<T>::iterator
insert_sorted( std::vector<T> &vec, T const &item ) { return vec.insert( std::upper_bound( vec.begin(), vec.end(), item ), item ); }

}


//------------------------------------------------------------------------
// extensions
//------------------------------------------------------------------------
namespace rz {

//檢查沒有相同的項目
template<typename T>
std::vector<T> &operator+=( std::vector<T> &l, const std::vector<T> &r ) {
	for ( const auto &item: r ) {
		if ( std::ranges::find( l.begin(), l.end(), item ) == l.end() ) l.push_back( item );
	}
	return l;
}

template<typename T>
std::set<T> &operator+=( std::set<T> &left, const std::set<T> &right ) {
	left.insert( right.begin(), right.end() );
	return left;
}

}
